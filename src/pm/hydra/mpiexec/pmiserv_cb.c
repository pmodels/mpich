/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "pmci.h"
#include "bsci.h"
#include "debugger.h"
#include "pmiserv.h"
#include "pmiserv_utils.h"
#include "pmiserv_common.h"
#include "pmiserv_pmi.h"

static HYD_status send_hdr_downstream(struct HYD_proxy *proxy, struct HYD_pmcd_hdr *hdr,
                                      void *buf, int buflen)
{
    HYD_status status = HYD_SUCCESS;

    hdr->pgid = proxy->pgid;
    hdr->proxy_id = proxy->proxy_id;
    hdr->buflen = buflen;

    int closed, sent;
    status = HYDU_sock_write(proxy->control_fd, hdr, sizeof(*hdr), &sent,
                             &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "sock write error\n");
    HYDU_ASSERT(!closed, status);

    if (buflen > 0) {
        status = HYDU_sock_write(proxy->control_fd, buf, buflen, &sent,
                                 &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "sock write error\n");
        HYDU_ASSERT(!closed, status);
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}


static HYD_status handle_pmi_cmd(struct HYD_proxy *proxy, int pgid, int process_fd, char *buf,
                                 int buflen, int pmi_version)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "[pgid: %d] got PMI command: %s\n", pgid, buf);

    struct PMIU_cmd pmi;
    status = PMIU_cmd_parse(buf, buflen, pmi_version, &pmi);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

#if defined ENABLE_PROFILING
    if (HYD_server_info.enable_profiling)
        HYD_server_info.num_pmi_calls++;
#endif /* ENABLE_PROFILING */

    switch (pmi.cmd_id) {
        case PMIU_CMD_SPAWN:
            status = HYD_pmiserv_spawn(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_PUBLISH:
            status = HYD_pmiserv_publish(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_UNPUBLISH:
            status = HYD_pmiserv_unpublish(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_LOOKUP:
            status = HYD_pmiserv_lookup(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_GET:
            status = HYD_pmiserv_kvs_get(proxy, process_fd, pgid, &pmi, false);
            break;
        case PMIU_CMD_KVSGET:
            status = HYD_pmiserv_kvs_get(proxy, process_fd, pgid, &pmi, true);
            break;
        case PMIU_CMD_KVSPUT:
            status = HYD_pmiserv_kvs_put(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_MPUT:
            /* internal put with multiple key/val pairs */
            status = HYD_pmiserv_kvs_mput(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_KVSFENCE:
            status = HYD_pmiserv_kvs_fence(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_BARRIER:
            /* barrier_in from proxy */
            status = HYD_pmiserv_barrier(proxy, process_fd, pgid, &pmi);
            break;
        case PMIU_CMD_ABORT:
            status = HYD_pmiserv_abort(proxy, process_fd, pgid, &pmi);
            break;
        default:
            /* We don't understand the command */
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "Unrecognized PMI %d command: %s | cleaning up processes\n",
                                pmi_version, pmi.cmd);
    }
    PMIU_cmd_free_buf(&pmi);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status cleanup_proxy(struct HYD_proxy *proxy)
{
    struct HYD_pg *pg = PMISERV_pg_by_id(proxy->pgid);
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

    /* If there is an active proxy, don't clean up the PG */
    for (int i = 0; i < pg->proxy_count; i++) {
        if (pg->proxy_list[i].control_fd != HYD_FD_CLOSED) {
            goto fn_exit;
        }
    }

    pg->is_active = false;

    /* If there is no active proxy, ignore the proxies that haven't
     * connected back to us yet. */
    /* HZ: seems redundant */
    for (int i = 0; i < pg->proxy_count; i++) {
        pg->proxy_list[i].control_fd = HYD_FD_CLOSED;
    }

    /* All proxies in this process group have terminated */
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* If this PG has already been closed, exit */
    if (pg_scratch->pmi_listen_fd == HYD_FD_CLOSED)
        goto fn_exit;

    /* If the PMI listen fd has been initialized, deregister it */
    if (pg_scratch->pmi_listen_fd != HYD_FD_UNSET && pg_scratch->pmi_listen_fd != HYD_FD_CLOSED) {
        status = HYDT_dmx_deregister_fd(pg_scratch->pmi_listen_fd);
        HYDU_ERR_POP(status, "unable to deregister PMI listen fd\n");
        close(pg_scratch->pmi_listen_fd);
    }
    pg_scratch->pmi_listen_fd = HYD_FD_CLOSED;

    /* If this is the main PG, free the debugger PID list */
    if (pg->pgid == 0)
        HYDT_dbg_free_procdesc();

    /* Reset the node allocations for this PG */
    for (int i = 0; i < pg->proxy_count; i++) {
        pg->proxy_list[i].node->active_processes -= pg->proxy_list[i].proxy_process_count;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmiserv_cleanup_all_pgs(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (int i = 0; i < PMISERV_pg_max_id(); i++) {
        struct HYD_pg *pg = PMISERV_pg_by_id(i);
        for (int j = 0; j < pg->proxy_count; j++) {
            status = cleanup_proxy(&pg->proxy_list[j]);
            HYDU_ERR_POP(status, "unable to cleanup proxy\n");
        }
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
    HYD_status status = HYD_SUCCESS;

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_SIGNAL;
    hdr.u.data = signum;

    status = send_hdr_downstream(proxy, &hdr, NULL, 0);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status control_cb(int fd, HYD_event_t events, void *userp)
{
    int count, closed;
    struct HYD_pmcd_hdr hdr;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int pgid, proxy_id;
    if (fd == STDIN_FILENO) {
        HYD_pmcd_init_header(&hdr);
        hdr.cmd = CMD_STDIN;
        /* assume stdin connected to the first pg and first proxy, but
         * should be configurable */
        pgid = 0;
        proxy_id = 0;
    } else {
        status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read command from proxy\n");
        HYDU_ASSERT(!closed, status);
        pgid = hdr.pgid;
        proxy_id = hdr.proxy_id;
    }

    struct HYD_proxy *proxy;
    struct HYD_pg *pg;
    pg = PMISERV_pg_by_id(pgid);
    proxy = &pg->proxy_list[proxy_id];
    HYDU_ASSERT(proxy == userp, status);

    if (hdr.cmd == CMD_PID_LIST) {      /* Got PIDs */
        HYDU_MALLOC_OR_JUMP(proxy->pid, int *, proxy->proxy_process_count * sizeof(int), status);
        status = HYDU_sock_read(fd, (void *) proxy->pid,
                                proxy->proxy_process_count * sizeof(int),
                                &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read status from proxy\n");
        HYDU_ASSERT(!closed, status);

        /* We initialize the debugger code only for non-dynamically
         * spawned processes and non-singleton */
        if (pgid == 0 && !HYD_server_info.is_singleton) {
            /* Check if all the PIDs have been received */
            for (int i = 0; i < pg->proxy_count; i++) {
                if (pg->proxy_list[i].pid == NULL) {
                    goto fn_exit;
                }
            }

            /* Call the debugger initialization */
            status = HYDT_dbg_setup_procdesc(pg);
            HYDU_ERR_POP(status, "debugger setup failed\n");
        }
    } else if (hdr.cmd == CMD_EXIT_STATUS) {
        HYDU_MALLOC_OR_JUMP(proxy->exit_status, int *, proxy->proxy_process_count * sizeof(int),
                            status);
        status =
            HYDU_sock_read(fd, (void *) proxy->exit_status,
                           proxy->proxy_process_count * sizeof(int), &count, &closed,
                           HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read status from proxy\n");
        HYDU_ASSERT(!closed, status);

        status = cleanup_proxy(proxy);
        HYDU_ERR_POP(status, "error cleaning up proxy connection\n");

        /* If any of the processes was killed with a signal, cleanup
         * the remaining processes */
        if (HYD_server_info.user_global.auto_cleanup) {
            for (int i = 0; i < proxy->proxy_process_count; i++) {
                if (!WIFEXITED(proxy->exit_status[i])) {
                    int code = proxy->exit_status[i];
                    /* show the value passed to exit(), not (val<<8) */
                    if (WIFEXITED(proxy->exit_status[i]))
                        code = WEXITSTATUS(proxy->exit_status[i]);

                    HYDU_dump_noprefix
                        (stdout, "\n==================================================");
                    HYDU_dump_noprefix(stdout, "=================================\n");
                    HYDU_dump_noprefix
                        (stdout, "=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES\n");
                    HYDU_dump_noprefix(stdout, "=   PID %d RUNNING AT %s\n", proxy->pid[i],
                                       proxy->node->hostname);
                    HYDU_dump_noprefix(stdout, "=   EXIT CODE: %d\n", code);
                    HYDU_dump_noprefix(stdout, "=   CLEANING UP REMAINING PROCESSES\n");
                    HYDU_dump_noprefix(stdout, "=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES\n");
                    HYDU_dump_noprefix(stdout,
                                       "==================================================");
                    HYDU_dump_noprefix(stdout, "=================================\n");

                    status = HYD_pmcd_pmiserv_cleanup_all_pgs();
                    HYDU_ERR_POP(status, "unable to cleanup processes\n");
                    break;
                }
            }
        }
    } else if (hdr.cmd == CMD_PMI) {
        HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen + 1, status);

        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI command from proxy\n");
        HYDU_ASSERT(!closed, status);

        buf[hdr.buflen] = 0;

        status = handle_pmi_cmd(proxy, pgid, hdr.u.pmi.process_fd, buf, hdr.buflen,
                                hdr.u.pmi.pmi_version);
        HYDU_ERR_POP(status, "unable to process PMI command\n");

        MPL_free(buf);
    } else if (hdr.cmd == CMD_STDOUT || hdr.cmd == CMD_STDERR) {
        HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen, status);

        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI command from proxy\n");
        HYDU_ASSERT(!closed, status);

        if (hdr.cmd == CMD_STDOUT) {
            status = HYD_server_info.stdout_cb(pgid, proxy_id, hdr.u.io.rank, buf, hdr.buflen);
        } else {
            status = HYD_server_info.stderr_cb(pgid, proxy_id, hdr.u.io.rank, buf, hdr.buflen);
        }
        HYDU_ERR_POP(status, "error in the UI defined callback\n");

        MPL_free(buf);
    } else if (hdr.cmd == CMD_STDIN) {
        HYDU_MALLOC_OR_JUMP(buf, char *, HYD_TMPBUF_SIZE, status);
        HYDU_ERR_POP(status, "unable to allocate memory\n");

        status =
            HYDU_sock_read(STDIN_FILENO, buf, HYD_TMPBUF_SIZE, &count, &closed,
                           HYDU_SOCK_COMM_NONE);
        HYDU_ERR_POP(status, "error reading from stdin\n");

        send_hdr_downstream(proxy, &hdr, buf, count);
        HYDU_ERR_POP(status, "error writing to control socket\n");

        if (!count) {
            status = HYDT_dmx_deregister_fd(STDIN_FILENO);
            HYDU_ERR_POP(status, "unable to deregister STDIN\n");
        }

        MPL_free(buf);
    } else if (hdr.cmd == CMD_PROCESS_TERMINATED) {
        int terminated_rank = hdr.u.data;
        if (HYD_server_info.user_global.auto_cleanup == 0) {
            /* Update the map of the alive processes */
            pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
            pg_scratch->dead_process_count++;

            if (pg_scratch->dead_process_count == 1) {
                /* This is the first dead process */
                MPL_free(pg_scratch->dead_processes);
                HYDU_MALLOC_OR_JUMP(pg_scratch->dead_processes, char *, PMI_MAXVALLEN, status);
                snprintf(pg_scratch->dead_processes, PMI_MAXVALLEN, "%d", terminated_rank);
            } else {
                /* FIXME: If the list of dead processes does not fit
                 * inside a single value length, set it as a
                 * multi-line value. */
                /* FIXME: This was changed from a sorted list where sequential
                 * numbers could be compacted to an expanded list where they
                 * couldn't. Obviously, this isn't sustainable on the PMI
                 * side, but on the MPI side, it's necessary (see the
                 * definition of MPI_COMM_FAILURE_ACK). In a future version of
                 * PMI where we can pass around things other than strings,
                 * this should improve. */
                char *segment, *current_list, *str;
                int included = 0, value;

                /* Create a sorted list of processes */
                current_list = MPL_strdup(pg_scratch->dead_processes);

                /* Search to see if this process is already in the list */
                included = 0;
                segment = strtok(current_list, ",");
                HYDU_ASSERT(segment != NULL, status);
                do {
                    value = strtol(segment, NULL, 10);
                    if (value == terminated_rank) {
                        included = 1;
                        break;
                    }
                    segment = strtok(NULL, ",");
                } while (segment != NULL);

                /* Add this process to the end of the list */
                if (!included) {
                    MPL_free(current_list);
                    HYDU_MALLOC_OR_JUMP(str, char *, PMI_MAXVALLEN, status);

                    snprintf(str, PMI_MAXVALLEN, "%s,%d", pg_scratch->dead_processes,
                             terminated_rank);
                } else {
                    str = current_list;
                }
                MPL_free(pg_scratch->dead_processes);
                pg_scratch->dead_processes = str;
            }

            /* HZ: do we need broadcast to other pg? */
            for (int i = 0; i < PMISERV_pg_max_id(); i++) {
                struct HYD_pg *tmp_pg = PMISERV_pg_by_id(i);
                for (int j = 0; j < tmp_pg->proxy_count; j++) {
                    struct HYD_proxy *tproxy = &tmp_pg->proxy_list[j];
                    if (tproxy->control_fd == HYD_FD_UNSET || tproxy->control_fd == HYD_FD_CLOSED)
                        continue;

                    if (tproxy == proxy)
                        continue;

                    status = HYD_pmcd_pmiserv_send_signal(tproxy, SIGUSR1);
                    HYDU_ERR_POP(status, "unable to send SIGUSR1 downstream\n");
                }
            }
        }
    } else {
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
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PROC_INFO;

    status = send_hdr_downstream(proxy, &hdr, NULL, 0);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

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
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    /* Read the proxy ID */
    struct HYD_pmcd_init_hdr init_hdr;
    int count, closed;
    status =
        HYDU_sock_read(fd, &init_hdr, sizeof(init_hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    if (status != HYD_SUCCESS || strcmp(init_hdr.signature, "HYD") != 0) {
        /* Ignore random connections (e.g. port scanner) */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to register fd\n");

        close(fd);
        goto fn_exit;
    }

    int pgid, proxy_id;
    pgid = init_hdr.pgid;
    proxy_id = init_hdr.proxy_id;

    /* Find the process group */
    struct HYD_pg *pg;
    pg = PMISERV_pg_by_id(pgid);
    if (!pg)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "could not find pg with ID %d\n", pgid);

    HYDU_ERR_CHKANDJUMP(status, !(proxy_id >= 0 && proxy_id < pg->proxy_count), HYD_INTERNAL_ERROR,
                        "cannot find proxy with ID %d\n", proxy_id);

    struct HYD_proxy *proxy;
    proxy = &pg->proxy_list[proxy_id];

    /* This will be the control socket for this proxy */
    proxy->control_fd = fd;

    /* Send out the executable information */
    status = send_exec_info(proxy);
    HYDU_ERR_POP(status, "unable to send exec info to proxy\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(1, &fd, HYD_POLLIN, proxy, control_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    if (pgid == 0 && proxy_id == 0) {
        int fd_stdin = STDIN_FILENO;
        int stdin_valid;
        struct HYD_pmcd_hdr hdr;

        status = HYDT_dmx_stdin_valid(&stdin_valid);
        HYDU_ERR_POP(status, "unable to check if stdin is valid\n");

        if (stdin_valid) {
            status = HYDT_dmx_register_fd(1, &fd_stdin, HYD_POLLIN, proxy, control_cb);
            HYDU_ERR_POP(status, "unable to register fd\n");
        } else {
            hdr.cmd = CMD_STDIN;
            status = send_hdr_downstream(proxy, &hdr, NULL, 0);
            HYDU_ERR_POP(status, "error writing to control socket\n");
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
    int accept_fd = -1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a control socket connection */
    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    status = HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, NULL, HYD_pmcd_pmiserv_proxy_init_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    if (-1 != accept_fd)
        close(accept_fd);
    goto fn_exit;
}

HYD_status HYD_control_listen(void)
{
    if (HYD_server_info.control_listen_fd == -1) {
        return HYDU_sock_create_and_listen_portstr(HYD_server_info.user_global.iface,
                                                   HYD_server_info.localhost,
                                                   HYD_server_info.port_range,
                                                   &HYD_server_info.control_port,
                                                   &HYD_server_info.control_listen_fd,
                                                   HYD_pmcd_pmiserv_control_listen_cb, NULL);
    } else {
        return HYD_SUCCESS;
    }
}
