/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
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

    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "[pgid: %d] got PMI command: %s\n", pgid, buf);

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, pmi_version, &cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

#if defined ENABLE_PROFILING
    if (HYD_handle.enable_profiling)
        HYD_handle.num_pmi_calls++;
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
    /* Cleanup all the processes */
    status = HYD_pmcd_pmiserv_cleanup();
    HYDU_ERR_POP(status, "unable to cleanup processes\n");
    goto fn_exit;
}

static HYD_status handle_pid_list(int fd, struct HYD_proxy *proxy)
{
    int count, closed;
    struct HYD_proxy *tproxy;
    struct HYD_pg *pg = proxy->pg;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(proxy->pid, int *, proxy->proxy_process_count * sizeof(int), status);
    status = HYDU_sock_read(fd, (void *) proxy->pid,
                            proxy->proxy_process_count * sizeof(int),
                            &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read status from proxy\n");
    HYDU_ASSERT(!closed, status);

    if (pg->pgid) {
        /* We initialize the debugger code only for non-dynamically
         * spawned processes */
        goto fn_exit;
    }

    /* Check if all the PIDs have been received */
    for (tproxy = pg->proxy_list; tproxy; tproxy = tproxy->next)
        if (tproxy->pid == NULL)
            goto fn_exit;

    /* Call the debugger initialization */
    status = HYDT_dbg_setup_procdesc(pg);
    HYDU_ERR_POP(status, "debugger setup failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status cleanup_proxy_connection(int fd, struct HYD_proxy *proxy)
{
    struct HYD_proxy *tproxy;
    struct HYD_pg *pg = proxy->pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "error deregistering fd\n");
    close(fd);

    /* Reset the control fd to -1, so when the fd is reused, we don't
     * find the wrong proxy */
    proxy->control_fd = -1;

    for (tproxy = pg->proxy_list; tproxy; tproxy = tproxy->next) {
        if (tproxy->exit_status == NULL)
            goto fn_exit;
    }

    /* All proxies in this process group have terminated */
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* If the PMI listen fd has been initialized, deregister it */
    if (pg_scratch->pmi_listen_fd != HYD_PMCD_PMI_FD_UNSET) {
        status = HYDT_dmx_deregister_fd(pg_scratch->pmi_listen_fd);
        HYDU_ERR_POP(status, "unable to deregister PMI listen fd\n");
        close(pg_scratch->pmi_listen_fd);
        pg_scratch->pmi_listen_fd = HYD_PMCD_PMI_FD_CLOSED;
    }

    if (pg_scratch->control_listen_fd != HYD_PMCD_PMI_FD_UNSET) {
        status = HYDT_dmx_deregister_fd(pg_scratch->control_listen_fd);
        HYDU_ERR_POP(status, "unable to deregister control listen fd\n");
        close(pg_scratch->control_listen_fd);
    }
    pg_scratch->control_listen_fd = HYD_PMCD_PMI_FD_CLOSED;

    /* If this is the main PG, free the debugger PID list */
    if (pg->pgid == 0)
        HYDT_dbg_free_procdesc();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_exit_status(int fd, struct HYD_proxy *proxy)
{
    int count, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(proxy->exit_status, int *, proxy->proxy_process_count * sizeof(int), status);
    status = HYDU_sock_read(fd, (void *) proxy->exit_status,
                            proxy->proxy_process_count * sizeof(int),
                            &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read status from proxy\n");
    HYDU_ASSERT(!closed, status);

    status = cleanup_proxy_connection(fd, proxy);
    HYDU_ERR_POP(status, "error cleaning up proxy connection\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status control_cb(int fd, HYD_event_t events, void *userp)
{
    int count, closed, i;
    enum HYD_pmcd_pmi_cmd cmd = INVALID_CMD;
    struct HYD_pmcd_pmi_hdr hdr;
    struct HYD_proxy *proxy;
    char *buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = (struct HYD_proxy *) userp;

    status = HYDU_sock_read(fd, &cmd, sizeof(cmd), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read command from proxy\n");

    if (closed) {
        /* this is only for developers; should not happen for regular
         * users */
        HYDU_dump(stderr, "connection to proxy terminated unexpectedly\n");

        /* allocate an exit status for the proxy, so we don't wait for it */
        HYDU_MALLOC(proxy->exit_status, int *, proxy->proxy_process_count * sizeof(int), status);
        for (i = 0; i < proxy->proxy_process_count; i++)
            proxy->exit_status[i] = 0;
        status = cleanup_proxy_connection(fd, proxy);
        HYDU_ERR_POP(status, "error cleaning up proxy connection\n");

        goto fn_exit;
    }

    if (cmd == PID_LIST) {      /* Got PIDs */
        status = handle_pid_list(fd, proxy);
        HYDU_ERR_POP(status, "unable to receive PID list\n");
    }
    else if (cmd == EXIT_STATUS) {
        status = handle_exit_status(fd, proxy);
        HYDU_ERR_POP(status, "unable to receive exit status\n");
    }
    else if (cmd == PMI_CMD) {
        status =
            HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI header from proxy\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC(buf, char *, hdr.buflen + 1, status);

        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI command from proxy\n");
        HYDU_ASSERT(!closed, status);

        buf[hdr.buflen] = 0;

        status = handle_pmi_cmd(fd, proxy->pg->pgid, hdr.pid, buf, hdr.pmi_version);
        HYDU_ERR_POP(status, "unable to process PMI command\n");

        HYDU_FREE(buf);
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unhandled command = %d\n", cmd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status send_exec_info(struct HYD_proxy *proxy)
{
    enum HYD_pmcd_pmi_cmd cmd;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = PROC_INFO;
    status =
        HYDU_sock_write(proxy->control_fd, &cmd, sizeof(enum HYD_pmcd_pmi_cmd), &sent,
                        &closed);
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
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next)
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
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next)
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

HYD_status HYD_pmcd_pmiserv_cleanup(void)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS, overall_status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        /* Close the control listen port, so new proxies cannot
         * connect back */
        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
        if (pg_scratch->control_listen_fd != HYD_PMCD_PMI_FD_UNSET) {
            status = HYDT_dmx_deregister_fd(pg_scratch->control_listen_fd);
            HYDU_ERR_POP(status, "unable to deregister control listen fd\n");
            close(pg_scratch->control_listen_fd);
        }
        pg_scratch->control_listen_fd = HYD_PMCD_PMI_FD_CLOSED;

        for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            /* The proxy has not been setup yet */
            if (proxy->control_fd == -1)
                continue;

            status = HYDT_dmx_deregister_fd(proxy->control_fd);
            HYDU_ERR_POP(status, "error deregistering fd\n");
            close(proxy->control_fd);

            proxy->control_fd = -1;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return overall_status;

  fn_fail:
    goto fn_exit;
}
