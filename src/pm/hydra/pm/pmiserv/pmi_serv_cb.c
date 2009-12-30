/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmi_handle.h"
#include "pmi_handle_common.h"
#include "pmci.h"
#include "bsci.h"
#include "debugger.h"
#include "pmi_serv.h"
#include "pmi_utils.h"

HYD_status HYD_pmcd_pmi_connect_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd, pgid;
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a PMI connection; find the PGID */
    pgid = ((int) (size_t) userp);
    for (pg = &HYD_handle.pg_list; pg && pg->pgid != pgid; pg = pg->next);
    if (!pg)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "cannot find pg with id %d\n", pgid);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
    pg_scratch->pmi_listen_fd = fd;

    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    status = HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, userp, HYD_pmcd_pmi_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    int linelen, i, j, k, cmdlen, pmi_version;
    char *buf = NULL, *tbuf = NULL, *cmd = NULL, *seg;
    char *tmp[HYD_NUM_TMP_STRINGS], *args[HYD_NUM_TMP_STRINGS], *targs[HYD_NUM_TMP_STRINGS];
    const char *delim;
    char *str1 = NULL, *str2 = NULL;
    struct HYD_pmcd_pmi_handle_fns *h;
    HYD_status status = HYD_SUCCESS;
    int buflen = 0, pgid;
    char *bufptr;

    HYDU_FUNC_ENTER();

    /* We got a PMI command; find the PGID */
    pgid = ((int) (size_t) userp);

    buflen = HYD_TMPBUF_SIZE;

    HYDU_MALLOC(buf, char *, buflen, status);
    bufptr = buf;

    /*
     * FIXME: This is a big hack. We temporarily initialize to
     * PMI-v1. If the incoming message is an "init", it will
     * reinitialize the function pointers. If we get an unsolicited
     * command, we just use the PMI-1 version for it.
     *
     * This part of the code should not know anything about PMI-1
     * vs. PMI-2. But the simple PMI client-side code is so hacked up,
     * that commands can arrive out-of-order and this is necessary.
     */
    if (HYD_pmcd_pmi_handle == NULL)
        HYD_pmcd_pmi_handle = &HYD_pmcd_pmi_v1;

    do {
        status = HYDU_sock_read(fd, bufptr, 6, &linelen, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read the length of the command");
        buflen -= linelen;
        bufptr += linelen;

        /* Unexpected termination of connection */
        if (linelen == 0)
            break;

        /* If we get "cmd=" here, we just assume that this is PMI-1
         * format (or a PMI-2 command that is backward compatible). */
        if (!strncmp(buf, "cmd=", strlen("cmd=")) || !strncmp(buf, "mcmd=", strlen("mcmd="))) {
            /* PMI-1 format command; read the rest of it */
            pmi_version = 1;

            if (!strncmp(buf, "cmd=", strlen("cmd=")))
                delim = " ";
            else
                delim = "\n";

            while (1) {
                status = HYDU_sock_readline(fd, bufptr, buflen, &linelen);
                HYDU_ERR_POP(status, "PMI read line error\n");
                buflen -= linelen;
                bufptr += linelen;

                if (!strncmp(buf, "cmd=", strlen("cmd=")) ||
                    !strncmp(bufptr - strlen("endcmd\n"), "endcmd", strlen("endcmd")))
                    break;
                else
                    *(bufptr - 1) = ' ';
            }

            /* Unexpected termination of connection */
            if (linelen == 0)
                break;
            else
                *(bufptr - 1) = '\0';

            /* Here we only get PMI-1 commands or backward compatible
             * PMI-2 commands, so we always explicitly use the PMI-1
             * delimiter. This allows us to get backward-compatible
             * PMI-2 commands interleaved with regular PMI-2
             * commands. */
            tbuf = HYDU_strdup(buf);
            cmd = strtok(tbuf, delim);
            for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
                targs[i] = strtok(NULL, delim);
                if (targs[i] == NULL)
                    break;
            }

            /* Make a pass through targs and merge space separated
             * arguments which are actually part of the same key */
            k = 0;
            for (i = 0; targs[i]; i++) {
                if (!strrchr(targs[i], ' ')) {
                    /* no spaces */
                    args[k++] = targs[i];
                }
                else {
                    /* space in the argument; each segment is either a
                     * new key, or a space-separated part of the
                     * previous key */
                    j = 0;
                    seg = strtok(targs[i], " ");
                    while (1) {
                        if (!seg || strrchr(seg, '=')) {
                            /* segment has an '='; it's a start of a new key */
                            if (j) {
                                tmp[j++] = NULL;
                                status = HYDU_str_alloc_and_join(tmp, &args[k++]);
                                HYDU_ERR_POP(status, "error while joining strings\n");
                                HYDU_free_strlist(tmp);
                            }
                            j = 0;

                            if (!seg)
                                break;
                        }
                        else {
                            /* no '='; part of the previous key */
                            tmp[j++] = HYDU_strdup(" ");
                        }
                        tmp[j++] = HYDU_strdup(seg);

                        seg = strtok(NULL, " ");
                    }
                }
            }
            args[k++] = NULL;

            if (!strcmp("cmd=init", cmd)) {
                /* Init is generic to all PMI implementations */
                status = HYD_pmcd_pmi_fn_init(fd, args);
                goto fn_exit;
            }
        }
        else {  /* PMI-2 command */
            pmi_version = 2;

            delim = ";";

            *bufptr = '\0';
            cmdlen = atoi(buf);

            status = HYDU_sock_read(fd, buf, cmdlen, &linelen, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "PMI read line error\n");
            buf[linelen] = 0;

            tbuf = HYDU_strdup(buf);
            cmd = strtok(tbuf, delim);
            for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
                args[i] = strtok(NULL, delim);
                if (args[i] == NULL)
                    break;
            }
        }
    } while (0);

    if (linelen == 0) {
        /* This is not a clean close. If a finalize was called, we
         * would have deregistered this socket. The application might
         * have aborted. Just cleanup all the processes */
        status = HYD_pmcd_pmi_serv_cleanup();
        if (status != HYD_SUCCESS) {
            HYDU_warn_printf("bootstrap server returned error cleaning up processes\n");
            status = HYD_SUCCESS;
            goto fn_fail;
        }

        status = HYDT_dmx_deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_warn_printf("unable to deregister fd %d\n", fd);
            status = HYD_SUCCESS;
            goto fn_fail;
        }

        close(fd);
        goto fn_exit;
    }

    /* Use the PMI version specific command delimiter to find what
     * command we got and call the appropriate handler
     * function. Before we get an "init", we are preinitialized to
     * PMI-1, so we will use that delimited even for PMI-2 for this
     * one command. From the next command onward, we will use the
     * PMI-2 specific delimiter. */
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "[pgid: %d] got PMI command: %s\n", pgid, buf);

    if (cmd == NULL) {
        status = HYD_SUCCESS;
    }
    else {
        /* Search for the PMI command in our table */
        status = HYDU_strsplit(cmd, &str1, &str2, '=');
        HYDU_ERR_POP(status, "string split returned error\n");

        h = HYD_pmcd_pmi_handle->handle_fns;
        while (h->handler) {
            if (!strcmp(str2, h->cmd)) {
                status = h->handler(fd, pgid, args);
                HYDU_ERR_POP(status, "PMI handler returned error\n");
                break;
            }
            h++;
        }
        if (!h->handler) {
            /* We don't understand the command */
            HYDU_error_printf("Unrecognized PMI command: %s | cleaning up processes\n", cmd);

            /* Cleanup all the processes and return. We don't need to
             * check the return status since we are anyway returning
             * an error */
            HYD_pmcd_pmi_serv_cleanup();
            HYDU_ERR_SETANDJUMP(status, HYD_SUCCESS, "");
        }
    }

  fn_exit:
    if (tbuf)
        HYDU_FREE(tbuf);
    if (buf)
        HYDU_FREE(buf);
    if (str1)
        HYDU_FREE(str1);
    if (str2)
        HYDU_FREE(str2);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_pmi_serv_control_connect_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd, proxy_id, count, pgid;
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a control socket connection */
    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    /* Get the PGID of the connection */
    pgid = ((int) (size_t) userp);

    /* Read the proxy ID */
    status = HYDU_sock_read(accept_fd, &proxy_id, sizeof(int), &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "sock read returned error\n");

    /* Find the process group */
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next)
        if (pg->pgid == pgid)
            break;
    if (!pg)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "could not find pg with ID %d\n",
                             pgid);

    /* Find the proxy */
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        if (proxy->proxy_id == proxy_id)
            break;
    }
    HYDU_ERR_CHKANDJUMP1(status, proxy == NULL, HYD_INTERNAL_ERROR,
                         "cannot find proxy with ID %d\n", proxy_id);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
    pg_scratch->control_listen_fd = fd;

    /* This will be the control socket for this proxy */
    proxy->control_fd = accept_fd;

    /* Send out the executable information */
    status = HYD_pmu_send_exec_info(proxy);
    HYDU_ERR_POP(status, "unable to send exec info to proxy\n");

    status =
        HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, proxy, HYD_pmcd_pmi_serv_control_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_serv_control_cb(int fd, HYD_event_t events, void *userp)
{
    struct HYD_proxy *proxy;
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_pmcd_pmi_process *process, *tmp;
    int count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = (struct HYD_proxy *) userp;
    pg = proxy->pg;

    if (proxy->pid == NULL) {   /* Got PIDs */
        HYDU_MALLOC(proxy->pid, int *, proxy->proxy_process_count * sizeof(int), status);
        status = HYDU_sock_read(fd, (void *) proxy->pid,
                                proxy->proxy_process_count * sizeof(int),
                                &count, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read status from proxy\n");

        if (pg->pgid) {
            /* We initialize the debugger code only for
             * non-dynamically spawned processes */
            goto fn_exit;
        }

        /* Check if all the PIDs have been received */
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            if (proxy->pid == NULL)
                goto fn_exit;

        /* Call the debugger initialization */
        status = HYDT_dbg_setup_procdesc(pg);
        HYDU_ERR_POP(status, "debugger setup failed\n");

        goto fn_exit;
    }

    HYDU_MALLOC(proxy->exit_status, int *, proxy->proxy_process_count * sizeof(int), status);
    status = HYDU_sock_read(fd, (void *) proxy->exit_status,
                            proxy->proxy_process_count * sizeof(int),
                            &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read status from proxy\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "error deregistering fd\n");

    close(fd);

    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        if (proxy->exit_status == NULL)
            goto fn_exit;
    }

    /* All proxies in this process group have terminated */
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* If the PMI listen fd has been initialized, deregister it */
    if (pg_scratch->pmi_listen_fd != -1) {
        status = HYDT_dmx_deregister_fd(pg_scratch->pmi_listen_fd);
        HYDU_ERR_POP(status, "unable to deregister PMI listen fd\n");
    }

    status = HYDT_dmx_deregister_fd(pg_scratch->control_listen_fd);
    HYDU_ERR_POP(status, "unable to deregister control listen fd\n");

    close(pg_scratch->pmi_listen_fd);
    close(pg_scratch->control_listen_fd);

    HYD_pmcd_pmi_free_pg_scratch(pg);

    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;

        if (proxy_scratch) {
            for (process = proxy_scratch->process_list; process;) {
                tmp = process->next;
                HYDU_FREE(process);
                process = tmp;
            }

            HYD_pmcd_free_pmi_kvs_list(proxy_scratch->kvs);
            HYDU_FREE(proxy_scratch);
            proxy->proxy_scratch = NULL;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_pmi_serv_cleanup(void)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    enum HYD_pmu_cmd cmd;
    HYD_status status = HYD_SUCCESS, overall_status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Instead of doing this from this process itself, fork a
     * bunch of processes to do this. */
    /* Connect to all proxies and send a KILL command */
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            cmd = KILL_JOB;
            status = HYDU_sock_trywrite(proxy->control_fd, &cmd, sizeof(enum HYD_pmu_cmd));
            if (status != HYD_SUCCESS) {
                HYDU_warn_printf("unable to send data to the proxy on %s\n",
                                 proxy->node.hostname);
                overall_status = HYD_INTERNAL_ERROR;
                continue;       /* Move on to the next proxy */
            }
        }
    }

    HYDU_FUNC_EXIT();

    return overall_status;
}


HYD_status HYD_pmcd_pmi_serv_ckpoint(void)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    enum HYD_pmu_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Instead of doing this from this process itself, fork a
     * bunch of processes to do this. */
    /* Connect to all proxies and send the checkpoint command */
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            cmd = CKPOINT;
            status = HYDU_sock_write(proxy->control_fd, &cmd, sizeof(enum HYD_pmu_cmd));
            HYDU_ERR_POP(status, "unable to send checkpoint message\n");
        }
    }

    HYDU_FUNC_EXIT();

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}


void HYD_pmcd_pmi_serv_signal_cb(int sig)
{
    HYDU_FUNC_ENTER();

    if (sig == SIGINT || sig == SIGQUIT || sig == SIGTERM) {
        /* There's nothing we can do with the return value for now. */
        HYD_pmcd_pmi_serv_cleanup();
    }
    else if (sig == SIGUSR1) {
        HYD_pmcd_pmi_serv_ckpoint();
    }
    /* Ignore other signals for now */

    HYDU_FUNC_EXIT();
    return;
}
