/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "pmip.h"
#include "demux.h"
#include "bsci.h"
#include "topo.h"

struct HYD_pmcd_pmip_s HYD_pmcd_pmip;

static HYD_status init_params(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_init_user_global(&HYD_pmcd_pmip.user_global);

    HYD_pmcd_pmip.upstream.server_name = NULL;
    HYD_pmcd_pmip.upstream.server_port = -1;
    HYD_pmcd_pmip.upstream.control = HYD_FD_UNSET;

    HYD_pmcd_pmip.local.id = -1;
    HYD_pmcd_pmip.local.pgid = -1;
    HYD_pmcd_pmip.local.retries = -1;

    PMIP_pg_init();

    return status;
}

static void cleanup_params(void)
{
    HYDU_finalize_user_global(&HYD_pmcd_pmip.user_global);


    /* Upstream */
    MPL_free(HYD_pmcd_pmip.upstream.server_name);


    PMIP_pg_finalize();
    HYDT_topo_finalize();
}

static void signal_cb(int sig)
{
    HYDU_FUNC_ENTER();

    if (sig == SIGPIPE) {
        /* Upstream socket closed; kill all processes */
        PMIP_bcast_signal(SIGKILL);
    } else if (sig == SIGTSTP) {
        PMIP_bcast_signal(sig);
    }
    /* Ignore other signals for now */

    HYDU_FUNC_EXIT();
    return;
}

static HYD_status pg_exit(struct pmip_pg *pg);

int main(int argc, char **argv)
{
    int pid, ret_status, sent, closed, ret, done;
    HYD_status status = HYD_SUCCESS;

    status = HYDU_dbg_init("proxy:unset");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    status = HYDU_set_signal(SIGPIPE, signal_cb);
    HYDU_ERR_POP(status, "unable to set SIGPIPE\n");

    status = HYDU_set_signal(SIGTSTP, signal_cb);
    HYDU_ERR_POP(status, "unable to set SIGTSTP\n");

    status = HYDU_set_common_signals(signal_cb);
    HYDU_ERR_POP(status, "unable to set common signals\n");

    status = init_params();
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

    status = HYD_pmcd_pmip_get_params(argv);
    HYDU_ERR_POP(status, "bad parameters passed to the proxy\n");

    status = HYDT_dmx_init(&HYD_pmcd_pmip.user_global.demux);
    HYDU_ERR_POP(status, "unable to initialize the demux engine\n");

    /* See if HYDI_CONTROL_FD is set before trying to connect upstream */
    ret = MPL_env2int("HYDI_CONTROL_FD", &HYD_pmcd_pmip.upstream.control);
    if (ret < 0) {
        HYDU_ERR_POP(status, "error reading HYDI_CONTROL_FD environment\n");
    } else if (ret == 0) {
        status = HYDU_sock_connect(HYD_pmcd_pmip.upstream.server_name,
                                   HYD_pmcd_pmip.upstream.server_port,
                                   &HYD_pmcd_pmip.upstream.control,
                                   HYD_pmcd_pmip.local.retries, HYD_CONNECT_DELAY);
        HYDU_ERR_POP(status,
                     "unable to connect to server %s at port %d (check for firewalls!)\n",
                     HYD_pmcd_pmip.upstream.server_name, HYD_pmcd_pmip.upstream.server_port);
    }

    struct HYD_pmcd_init_hdr init_hdr;
    strncpy(init_hdr.signature, "HYD", 4);
    init_hdr.pgid = HYD_pmcd_pmip.local.pgid;
    init_hdr.proxy_id = HYD_pmcd_pmip.local.id;
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             &init_hdr, sizeof(init_hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send the proxy ID to the server\n");
    if (closed)
        goto fn_fail;

    status = HYDT_dmx_register_fd(1, &HYD_pmcd_pmip.upstream.control,
                                  HYD_POLLIN, NULL, HYD_pmcd_pmip_control_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    done = 0;
    while (1) {
        /* Wait for some event to occur */
        status = HYDT_dmx_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");

        if (!PMIP_pg_0()) {
            /* processes haven't been launched yet */
            continue;
        }

        /* Exit the loop if no open read socket left */
        if (!PMIP_has_open_stdoe()) {
            break;
        }

        pid = waitpid(-1, &ret_status, WNOHANG);

        if (pid > 0) {
            struct pmip_downstream *p = PMIP_find_downstream_by_pid(pid);
            if (p) {
                p->exit_status = ret_status;
                if (WIFSIGNALED(ret_status)) {
                    /* kill all processes */
                    PMIP_bcast_signal(SIGKILL);
                }
                done++;
            }
        }
    }

    /* collect the singleton's exit_status */
    if (PMIP_pg_0()->is_singleton) {
        struct pmip_pg *pg_0 = PMIP_pg_0();

        HYDU_ASSERT(pg_0->num_procs == 1, status);
        HYDU_ASSERT(pg_0->downstreams[0].pid == HYD_pmcd_pmip.singleton_pid, status);
        /* We won't get the singleton's exit status. Assume it's 0. */
        if (pg_0->downstreams[0].exit_status == PMIP_EXIT_STATUS_UNSET) {
            pg_0->downstreams[0].exit_status = 0;
        }

        done++;
    }

    /* Wait for the processes to finish */
    int total_count = PMIP_get_total_process_count();
    while (done < total_count) {
        pid = waitpid(-1, &ret_status, 0);

        /* Find the pid and mark it as complete. */
        if (pid > 0) {
            struct pmip_downstream *tmp = PMIP_find_downstream_by_pid(pid);
            if (tmp) {
                if (tmp->exit_status == PMIP_EXIT_STATUS_UNSET) {
                    tmp->exit_status = ret_status;
                }
                done++;
            }
        }

        /* Check if there are any messages from the launcher */
        status = HYDT_dmx_wait_for_event(0);
        HYDU_IGNORE_TIMEOUT(status);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");
    }

    status = PMIP_foreach_pg_do(pg_exit);
    HYDU_ERR_POP(status, "error sending exit statuses for each pg\n");

    status = HYDT_dmx_deregister_fd(HYD_pmcd_pmip.upstream.control);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(HYD_pmcd_pmip.upstream.control);

    status = HYDT_dmx_finalize();
    HYDU_ERR_POP(status, "error returned from demux finalize\n");

    status = HYDT_bsci_finalize();
    HYDU_ERR_POP(status, "unable to finalize the bootstrap device\n");

    /* cleanup the params structure */
    cleanup_params();

  fn_exit:
    HYDU_dbg_finalize();
    return status;

  fn_fail:
    /* kill all processes */
    PMIP_bcast_signal(SIGKILL);
    goto fn_exit;
}

static HYD_status pg_exit(struct pmip_pg *pg)
{
    HYD_status status = HYD_SUCCESS;

    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_EXIT_STATUS;

    int *exit_status_list;
    exit_status_list = PMIP_pg_get_exit_status_list(pg);
    PMIP_send_hdr_upstream(pg, &hdr, exit_status_list, pg->num_procs * sizeof(int));
    HYDU_ERR_POP(status, "unable to send EXIT_STATUS command upstream for proxy %d - %d\n",
                 pg->pgid, pg->proxy_id);
    MPL_free(exit_status_list);

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}
