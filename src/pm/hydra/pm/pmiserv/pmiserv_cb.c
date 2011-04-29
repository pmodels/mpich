/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "pmci.h"
#include "bsci.h"
#include "debugger.h"
#include "pmiserv.h"
#include "pmiserv_utils.h"
#include "pmiserv_pmi.h"

static HYD_status handle_pmi_cmd(int fd, int pgid, int pid, char *buf, int pmi_version)
{
    char *args[HYD_NUM_TMP_STRINGS], *cmd = NULL;
    struct HYD_pmcd_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pmi_version == 1)
        HYD_pmcd_pmi_handle = HYD_pmcd_pmi_v1;
    else
        HYD_pmcd_pmi_handle = HYD_pmcd_pmi_v2;

    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "[pgid: %d] got PMI command: %s\n", pgid, buf);

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, pmi_version, &cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

#if defined ENABLE_PROFILING
    if (HYD_server_info.enable_profiling)
        HYD_server_info.num_pmi_calls++;
#endif /* ENABLE_PROFILING */

    h = HYD_pmcd_pmi_handle;
    while (h->handler) {
        if (!strcmp(cmd, h->cmd)) {
            status = h->handler(fd, pid, pgid, args);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            break;
        }
        h++;
    }
    if (!h->handler) {
        /* We don't understand the command */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Unrecognized PMI command: %s | cleaning up processes\n", cmd);
    }

  fn_exit:
    if (cmd)
        HYDU_FREE(cmd);
    HYDU_free_strlist(args);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status cleanup_proxy(struct HYD_proxy *proxy)
{
    struct HYD_proxy *tproxy;
    struct HYD_pg *pg = proxy->pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (proxy->control_fd != HYD_FD_UNSET && proxy->control_fd != HYD_FD_CLOSED) {
        status = HYDT_dmx_deregister_fd(proxy->control_fd);
        HYDU_ERR_POP(status, "error deregistering fd\n");
        close(proxy->control_fd);

        /* Reset the control fd, so when the fd is reused, we don't
         * find the wrong proxy */
        proxy->control_fd = HYD_FD_CLOSED;
    }
    else {
        /* We go through the following cleanup only if we actually
         * cleaned up a proxy. Otherwise, once all the proxies in a PG
         * are cleaned up, every time this function is called, we
         * might try to cleanup the resources. */
        goto fn_exit;
    }

    for (tproxy = pg->proxy_list; tproxy; tproxy = tproxy->next)
        if (tproxy->control_fd != HYD_FD_CLOSED)
            goto fn_exit;

    /* All proxies in this process group have terminated */
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* If the PMI listen fd has been initialized, deregister it */
    if (pg_scratch->pmi_listen_fd != HYD_FD_UNSET &&
        pg_scratch->pmi_listen_fd != HYD_FD_CLOSED) {
        status = HYDT_dmx_deregister_fd(pg_scratch->pmi_listen_fd);
        HYDU_ERR_POP(status, "unable to deregister PMI listen fd\n");
        close(pg_scratch->pmi_listen_fd);
    }
    pg_scratch->pmi_listen_fd = HYD_FD_CLOSED;

    if (pg_scratch->control_listen_fd != HYD_FD_UNSET &&
        pg_scratch->control_listen_fd != HYD_FD_CLOSED) {
        status = HYDT_dmx_deregister_fd(pg_scratch->control_listen_fd);
        HYDU_ERR_POP(status, "unable to deregister control listen fd\n");
        close(pg_scratch->control_listen_fd);
    }
    pg_scratch->control_listen_fd = HYD_FD_CLOSED;

    /* If this is the main PG, free the debugger PID list */
    if (pg->pgid == 0)
        HYDT_dbg_free_procdesc();

    /* Reset the node allocations for this PG */
    for (tproxy = pg->proxy_list; tproxy; tproxy = tproxy->next)
        tproxy->node->active_processes -= tproxy->proxy_process_count;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status cleanup_pg(struct HYD_pg *pg)
{
    struct HYD_proxy *proxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        status = cleanup_proxy(proxy);
        HYDU_ERR_POP(status, "unable to cleanup proxy\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmiserv_cleanup_all_pgs(void)
{
    struct HYD_pg *pg;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (pg = &HYD_server_info.pg_list; pg; pg = pg->next) {
        status = cleanup_pg(pg);
        HYDU_ERR_POP(status, "unable to cleanup PG\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmiserv_send_signal(struct HYD_proxy *proxy, int signum)
{
    struct HYD_pmcd_hdr hdr;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = SIGNAL;
    hdr.signum = signum;

    status = HYDU_sock_write(proxy->control_fd, &hdr, sizeof(hdr), &sent, &closed);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status control_cb(int fd, HYD_event_t events, void *userp)
{
    int count, closed, i;
    struct HYD_pg *pg;
    struct HYD_pmcd_hdr hdr;
    struct HYD_proxy *proxy, *tproxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = (struct HYD_proxy *) userp;

    if (fd == STDIN_FILENO) {
        HYD_pmcd_init_header(&hdr);
        hdr.cmd = STDIN;
    }
    else {
        status =
            HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read command from proxy\n");
        HYDU_ASSERT(!closed, status);
    }

    if (hdr.cmd == PID_LIST) {  /* Got PIDs */
        HYDU_MALLOC(proxy->pid, int *, proxy->proxy_process_count * sizeof(int), status);
        status = HYDU_sock_read(fd, (void *) proxy->pid,
                                proxy->proxy_process_count * sizeof(int),
                                &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read status from proxy\n");
        HYDU_ASSERT(!closed, status);

        if (proxy->pg->pgid) {
            /* We initialize the debugger code only for non-dynamically
             * spawned processes */
            goto fn_exit;
        }

        /* Check if all the PIDs have been received */
        for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next)
            if (tproxy->pid == NULL)
                goto fn_exit;

        /* Call the debugger initialization */
        status = HYDT_dbg_setup_procdesc(proxy->pg);
        HYDU_ERR_POP(status, "debugger setup failed\n");
    }
    else if (hdr.cmd == EXIT_STATUS) {
        HYDU_MALLOC(proxy->exit_status, int *, proxy->proxy_process_count * sizeof(int),
                    status);
        status =
            HYDU_sock_read(fd, (void *) proxy->exit_status,
                           proxy->proxy_process_count * sizeof(int), &count, &closed,
                           HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read status from proxy\n");
        HYDU_ASSERT(!closed, status);

        status = cleanup_proxy(proxy);
        HYDU_ERR_POP(status, "error cleaning up proxy connection\n");

        /* If any of the processes aborted, cleanup the remaining
         * processes */
        if (HYD_server_info.user_global.auto_cleanup) {
            for (i = 0; i < proxy->proxy_process_count; i++) {
                if (!WIFEXITED(proxy->exit_status[i])) {
                    HYDU_dump_noprefix
                        (stdout, "\n====================================================");
                    HYDU_dump_noprefix(stdout, "=================================\n");
                    HYDU_dump_noprefix
                        (stdout, "=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES\n");
                    HYDU_dump_noprefix(stdout, "=   EXIT CODE: %d\n", proxy->exit_status[i]);
                    HYDU_dump_noprefix(stdout, "=   CLEANING UP REMAINING PROCESSES\n");
                    HYDU_dump_noprefix(stdout,
                                       "=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES\n");
                    HYDU_dump_noprefix(stdout,
                                       "====================================================");
                    HYDU_dump_noprefix(stdout, "=================================\n");

                    status = HYD_pmcd_pmiserv_cleanup_all_pgs();
                    HYDU_ERR_POP(status, "unable to cleanup processes\n");
                    break;
                }
            }
        }
    }
    else if (hdr.cmd == PMI_CMD) {
        HYDU_MALLOC(buf, char *, hdr.buflen + 1, status);

        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI command from proxy\n");
        HYDU_ASSERT(!closed, status);

        buf[hdr.buflen] = 0;

        status = handle_pmi_cmd(fd, proxy->pg->pgid, hdr.pid, buf, hdr.pmi_version);
        HYDU_ERR_POP(status, "unable to process PMI command\n");

        HYDU_FREE(buf);
    }
    else if (hdr.cmd == STDOUT || hdr.cmd == STDERR) {
        HYDU_MALLOC(buf, char *, hdr.buflen, status);

        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI command from proxy\n");
        HYDU_ASSERT(!closed, status);

        if (hdr.cmd == STDOUT)
            status =
                HYD_server_info.stdout_cb(hdr.pgid, hdr.proxy_id, hdr.rank, buf, hdr.buflen);
        else
            status =
                HYD_server_info.stderr_cb(hdr.pgid, hdr.proxy_id, hdr.rank, buf, hdr.buflen);
        HYDU_ERR_POP(status, "error in the UI defined callback\n");

        HYDU_FREE(buf);
    }
    else if (hdr.cmd == STDIN) {
        HYDU_MALLOC(buf, char *, HYD_TMPBUF_SIZE, status);
        HYDU_ERR_POP(status, "unable to allocate memory\n");

        HYDU_sock_read(STDIN_FILENO, buf, HYD_TMPBUF_SIZE, &count, &closed, 0);
        HYDU_ERR_POP(status, "error reading from stdin\n");

        hdr.buflen = count;

        HYDU_sock_write(proxy->control_fd, &hdr, sizeof(hdr), &count, &closed);
        HYDU_ERR_POP(status, "error writing to control socket\n");
        HYDU_ASSERT(!closed, status);

        if (hdr.buflen) {
            HYDU_sock_write(proxy->control_fd, buf, hdr.buflen, &count, &closed);
            HYDU_ERR_POP(status, "error writing to control socket\n");
            HYDU_ASSERT(!closed, status);

            HYDU_FREE(buf);
        }
        else {
            status = HYDT_dmx_deregister_fd(STDIN_FILENO);
            HYDU_ERR_POP(status, "unable to deregister STDIN\n");
        }
    }
    else if (hdr.cmd == PROCESS_TERMINATED) {
        if (HYD_server_info.user_global.auto_cleanup == 0) {
            /* Update the map of the alive processes */
            pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;
            pg_scratch->dead_process_count++;

            if (pg_scratch->dead_process_count == 1) {
                /* This is the first dead process */
                HYDU_FREE(pg_scratch->dead_processes);
                HYDU_MALLOC(pg_scratch->dead_processes, char *, PMI_MAXVALLEN, status);
                HYDU_snprintf(pg_scratch->dead_processes, PMI_MAXVALLEN, "%d", hdr.pid);
            }
            else {
                /* FIXME: If the list of dead processes does not fit
                 * inside a single value length, set it as a
                 * multi-line value. */
                struct proc_list {
                    char *segment;
                    int start;
                    int end;
                    struct proc_list *next;
                } *list = NULL, *e, *tmp;
                char *segment, *start, *end, *current_list, *str, *run;
                int included = 0;

                /* Create a sorted list of processes */
                current_list = HYDU_strdup(pg_scratch->dead_processes);

                segment = strtok(current_list, ",");
                do {
                    HYDU_MALLOC(e, struct proc_list *, sizeof(struct proc_list), status);
                    e->segment = HYDU_strdup(segment);
                    e->next = NULL;

                    if (list == NULL)
                        list = e;
                    else {
                        for (tmp = list; tmp->next; tmp = tmp->next);
                        tmp->next = e;
                    }

                    segment = strtok(NULL, ",");
                } while (segment);

                for (e = list; e; e = e->next) {
                    start = strtok(e->segment, "-");
                    end = strtok(NULL, "-");

                    e->start = atoi(start);
                    if (end)
                        e->end = atoi(end);
                    else
                        e->end = atoi(start);

                    if (hdr.pid == e->start - 1) {
                        e->start = hdr.pid;
                        included = 1;
                    }
                    else if (hdr.pid == e->end + 1) {
                        e->end = hdr.pid;
                        included = 1;
                    }
                }

                if (!included) {
                    HYDU_MALLOC(e, struct proc_list *, sizeof(struct proc_list), status);
                    e->start = hdr.pid;
                    e->end = hdr.pid;
                    e->next = NULL;

                    if (hdr.pid < list->start) {
                        e->next = list;
                        list = e;
                    }
                    else {
                        for (tmp = list; tmp->next && tmp->next->start < hdr.pid;
                             tmp = tmp->next);
                        e->next = tmp->next;
                        tmp->next = e;
                    }
                }

                for (e = list; e->next;) {
                    if (e->end == e->next->start) {
                        e->end = e->next->end;
                        tmp = e->next;
                        e->next = e->next->next;
                        HYDU_FREE(tmp);
                    }
                    else
                        e = e->next;
                }

                str = NULL;
                for (e = list; e; e = e->next) {
                    HYDU_MALLOC(run, char *, PMI_MAXVALLEN, status);
                    if (str) {
                        if (e->start == e->end)
                            HYDU_snprintf(run, PMI_MAXVALLEN, "%s,%d", str, e->start);
                        else
                            HYDU_snprintf(run, PMI_MAXVALLEN, "%s,%d-%d", str, e->start,
                                          e->end);
                    }
                    else {
                        if (e->start == e->end)
                            HYDU_snprintf(run, PMI_MAXVALLEN, "%d", e->start);
                        else
                            HYDU_snprintf(run, PMI_MAXVALLEN, "%d-%d", e->start, e->end);
                    }
                    if (str)
                        HYDU_FREE(str);
                    str = run;
                }
                pg_scratch->dead_processes = str;
            }

            for (pg = &HYD_server_info.pg_list; pg; pg = pg->next) {
                for (tproxy = pg->proxy_list; tproxy; tproxy = tproxy->next) {
                    if (tproxy->control_fd == HYD_FD_UNSET ||
                        tproxy->control_fd == HYD_FD_CLOSED)
                        continue;

                    if (tproxy->pg->pgid == proxy->pg->pgid &&
                        tproxy->proxy_id == proxy->proxy_id)
                        continue;

                    status = HYD_pmcd_pmiserv_send_signal(tproxy, SIGUSR1);
                    HYDU_ERR_POP(status, "unable to send SIGUSR1 downstream\n");
                }
            }
        }
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unhandled command = %d\n", hdr.cmd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status send_exec_info(struct HYD_proxy *proxy)
{
    struct HYD_pmcd_hdr hdr;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PROC_INFO;
    status = HYDU_sock_write(proxy->control_fd, &hdr, sizeof(hdr), &sent, &closed);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_send_strlist(proxy->control_fd, proxy->exec_launch_info);
    HYDU_ERR_POP(status, "error sending exec info\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmiserv_proxy_init_cb(int fd, HYD_event_t events, void *userp)
{
    int proxy_id, count, pgid, closed;
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Get the PGID of the connection */
    pgid = ((int) (size_t) userp);

    /* Read the proxy ID */
    status = HYDU_sock_read(fd, &proxy_id, sizeof(int), &count, &closed,
                            HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "sock read returned error\n");
    HYDU_ASSERT(!closed, status);

    /* Find the process group */
    for (pg = &HYD_server_info.pg_list; pg; pg = pg->next)
        if (pg->pgid == pgid)
            break;
    if (!pg)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "could not find pg with ID %d\n",
                            pgid);

    /* Find the proxy */
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        if (proxy->proxy_id == proxy_id)
            break;
    }
    HYDU_ERR_CHKANDJUMP(status, proxy == NULL, HYD_INTERNAL_ERROR,
                        "cannot find proxy with ID %d\n", proxy_id);

    /* This will be the control socket for this proxy */
    proxy->control_fd = fd;

    /* Send out the executable information */
    status = send_exec_info(proxy);
    HYDU_ERR_POP(status, "unable to send exec info to proxy\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(1, &fd, HYD_POLLIN, proxy, control_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    if (pgid == 0 && proxy->proxy_id == 0) {
        int fd_stdin = STDIN_FILENO;
        int stdin_valid;
        struct HYD_pmcd_hdr hdr;

        status = HYDT_dmx_stdin_valid(&stdin_valid);
        HYDU_ERR_POP(status, "unable to check if stdin is valid\n");

        if (stdin_valid) {
            status = HYDT_dmx_register_fd(1, &fd_stdin, HYD_POLLIN, proxy, control_cb);
            HYDU_ERR_POP(status, "unable to register fd\n");
        }
        else {
            hdr.cmd = STDIN;
            hdr.buflen = 0;
            HYDU_sock_write(proxy->control_fd, &hdr, sizeof(hdr), &count, &closed);
            HYDU_ERR_POP(status, "error writing to control socket\n");
            HYDU_ASSERT(!closed, status);
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmiserv_control_listen_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd, pgid;
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Get the PGID of the connection */
    pgid = ((int) (size_t) userp);

    /* Find the process group */
    for (pg = &HYD_server_info.pg_list; pg; pg = pg->next)
        if (pg->pgid == pgid)
            break;
    if (!pg)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "could not find pg with ID %d\n",
                            pgid);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
    pg_scratch->control_listen_fd = fd;

    /* We got a control socket connection */
    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    status = HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, userp,
                                  HYD_pmcd_pmiserv_proxy_init_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
