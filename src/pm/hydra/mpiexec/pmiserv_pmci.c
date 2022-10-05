/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "pmci.h"
#include "pmiserv_pmi.h"
#include "bsci.h"
#include "bscu.h"
#include "pmiserv.h"
#include "pmiserv_utils.h"

static HYD_status ui_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    struct HYD_cmd cmd;
    int count, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &cmd, sizeof(cmd), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "read error\n");
    HYDU_ASSERT(!closed, status);

    if (cmd.type == HYD_CLEANUP) {
        HYDU_dump_noprefix(stdout, "Ctrl-C caught... cleaning up processes\n");
        status = HYD_pmcd_pmiserv_cleanup_all_pgs();
        HYDU_ERR_POP(status, "cleanup of processes failed\n");

        /* Force kill all bootstrap processes that we launched */
        status = HYDT_bsci_wait_for_completion(0);
        HYDU_ERR_POP(status, "launcher returned error waiting for completion\n");

        exit(1);
    } else if (cmd.type == HYD_SIGCHLD) {
        pid_t pid;
        int ret;
        while (1) {
            pid = waitpid(-1, &ret, WNOHANG);
            if (pid <= 0) {
                break;
            }
            struct HYD_proxy_pid *p;
            p = HYDT_bscu_pid_list_find(pid);
            if (p) {
                p->pid = -1;
                if (p->proxy) {
                    if (p->proxy->control_fd == HYD_FD_UNSET) {
                        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "Launch proxy failed.\n");
                    }
                }
            }
        }
    } else if (cmd.type == HYD_SIGNAL) {
        for (int i = 0; i < PMISERV_pg_max_id(); i++) {
            struct HYD_pg *pg = PMISERV_pg_by_id(i);
            for (int j = 0; j < pg->proxy_count; j++) {
                status = HYD_pmcd_pmiserv_send_signal(&pg->proxy_list[j], cmd.signum);
                HYDU_ERR_POP(status, "unable to send signal downstream\n");
            }
        }
    } else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized command\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_launch_procs(void)
{
    struct HYD_string_stash proxy_stash;
    char *control_port = NULL;
    int node_count, i, *control_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy_stash.strlist = NULL;

    status = HYDT_dmx_register_fd(1, &HYD_server_info.cmd_pipe[0], POLLIN, NULL, ui_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    struct HYD_pg *pg;
    pg = PMISERV_pg_by_id(0);
    status = HYD_pmcd_pmi_alloc_pg_scratch(pg);
    HYDU_ERR_POP(status, "error allocating pg scratch space\n");

    /* PMI-v2 kvs-fence */
    status = HYD_pmiserv_epoch_init(pg);
    HYDU_ERR_POP(status, "unable to init epoch\n");

    int listen_fd;
    status = HYDU_sock_create_and_listen_portstr(HYD_server_info.user_global.iface,
                                                 HYD_server_info.localhost,
                                                 HYD_server_info.port_range, &control_port,
                                                 &listen_fd,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) pgid);
    HYDU_ERR_POP(status, "unable to create PMI port\n");

    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) PMISERV_pg_0->pg_scratch;
    pg_scratch->control_listen_fd = listen_fd;

    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    status = HYD_pmcd_pmi_fill_in_proxy_args(&proxy_stash, control_port, 0);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pg);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    node_count = pg->proxy_count;

    HYDU_MALLOC_OR_JUMP(control_fd, int *, node_count * sizeof(int), status);
    for (i = 0; i < node_count; i++)
        control_fd[i] = HYD_FD_UNSET;

    status = HYDT_bsci_launch_procs(proxy_stash.strlist, pg->proxy_list, pg->proxy_count,
                                    HYD_TRUE, control_fd);
    HYDU_ERR_POP(status, "launcher cannot launch processes\n");

    for (i = 0; i < pg->proxy_count; i++) {
        if (control_fd[i] != HYD_FD_UNSET) {
            pg->proxy_list[i].control_fd = control_fd[i];

            status = HYDT_dmx_register_fd(1, &control_fd[i], HYD_POLLIN, (void *) (size_t) pgid,
                                          HYD_pmcd_pmiserv_proxy_init_cb);
            HYDU_ERR_POP(status, "unable to register fd\n");
        }
    }

    MPL_free(control_fd);

  fn_exit:
    MPL_free(control_port);
    HYD_STRING_STASH_FREE(proxy_stash);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_wait_for_completion(int timeout)
{
    int time_elapsed, time_left;
    struct timeval start, now;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    gettimeofday(&start, NULL);

    /* We first wait for the exit statuses to arrive till the timeout
     * period */
    for (int i = 0; i < PMISERV_pg_max_id(); i++) {
        /* NOTE: avoid grab pg pointer because we use dynamic array for pg_list,
         *       the pg pointer may not be persistent during the event loop */
        while (PMISERV_pg_by_id(i)->is_active) {
            gettimeofday(&now, NULL);
            time_elapsed = (now.tv_sec - start.tv_sec);
            time_left = timeout;
            if (timeout > 0) {
                if (time_elapsed > timeout) {
                    HYDU_dump(stdout, "APPLICATION TIMED OUT, TIMEOUT = %ds\n", timeout);

                    status = HYD_pmcd_pmiserv_cleanup_all_pgs();
                    HYDU_ERR_POP(status, "cleanup of processes failed\n");

                    /* Force kill all bootstrap processes that we launched */
                    status = HYDT_bsci_wait_for_completion(0);
                    HYDU_ERR_POP(status, "launcher returned error waiting for completion\n");
                } else
                    time_left = timeout - time_elapsed;
            }

            status = HYDT_dmx_wait_for_event(time_left);
            if (status == HYD_TIMED_OUT)
                continue;
            HYDU_ERR_POP(status, "error waiting for event\n");
        }
    }

    /* Either all application processes exited or we have timed out.
     * We now wait for all the proxies to terminate. */
    gettimeofday(&now, NULL);
    time_elapsed = (now.tv_sec - start.tv_sec);
    if (timeout > 0) {
        time_left = timeout - time_elapsed;
        if (time_left < 0)
            time_left = 0;
    } else
        time_left = timeout;

    status = HYDT_bsci_wait_for_completion(time_left);
    HYDU_ERR_POP(status, "launcher returned error waiting for completion\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_finalize();
    HYDU_ERR_POP(status, "unable to finalize process manager utils\n");

    status = HYDT_bsci_finalize();
    HYDU_ERR_POP(status, "unable to finalize bootstrap server\n");

    status = HYDT_dmx_finalize();
    HYDU_ERR_POP(status, "error returned from demux finalize\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
