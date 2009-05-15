/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmi_handle.h"
#include "pmi_handle_common.h"
#include "pmi_handle_v1.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"
#include "pmi_serv.h"

HYD_Handle handle;
struct HYD_PMCD_pmi_handle *HYD_PMCD_pmi_handle;

HYD_Status HYD_PMCD_pmi_connect_cb(int fd, HYD_Event_t events, void *userp)
{
    int accept_fd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a PMI connection */
    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    status = HYD_DMX_register_fd(1, &accept_fd, HYD_STDOUT, NULL, HYD_PMCD_pmi_cmd_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_cmd_cb(int fd, HYD_Event_t events, void *userp)
{
    int linelen, i, cmdlen;
    char *buf = NULL, *tbuf = NULL, *cmd, *args[HYD_NUM_TMP_STRINGS];
    char *str1 = NULL, *str2 = NULL;
    struct HYD_PMCD_pmi_handle_fns *h;
    HYD_Status status = HYD_SUCCESS;
    int buflen = 0;
    char *bufptr;

    HYDU_FUNC_ENTER();

    /* We got a PMI command */

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
    if (HYD_PMCD_pmi_handle == NULL)
        HYD_PMCD_pmi_handle = HYD_PMCD_pmi_v1;

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
        if (!strncmp(buf, "cmd=", strlen("cmd="))) {
            /* PMI-1 format command; read the rest of it */
            status = HYDU_sock_readline(fd, bufptr, buflen, &linelen);
            HYDU_ERR_POP(status, "PMI read line error\n");
            buflen -= linelen;
            bufptr += linelen;

            /* Unexpected termination of connection */
            if (linelen == 0)
                break;
            else 
                *(bufptr-1) = '\0';

            /* Here we only get PMI-1 commands or backward compatible
             * PMI-2 commands, so we always explicitly use the PMI-1
             * delimiter. This allows us to get backward-compatible
             * PMI-2 commands interleaved with regular PMI-2
             * commands. */
            tbuf = HYDU_strdup(buf);
            cmd = strtok(tbuf, HYD_PMCD_pmi_v1->delim);
            for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
                args[i] = strtok(NULL, HYD_PMCD_pmi_v1->delim);
                if (args[i] == NULL)
                    break;
            }

            if (!strcmp("cmd=init", cmd)) {
                /* Init is generic to all PMI implementations */
                status = HYD_PMCD_pmi_handle_init(fd, args);
                goto fn_exit;
            }
        }
        else {  /* PMI-2 command */
            *bufptr = '\0';
            cmdlen = atoi(buf);

            status = HYDU_sock_read(fd, buf, cmdlen, &linelen, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "PMI read line error\n");
            buf[linelen] = 0;
        }
    } while (0);

    if (linelen == 0) {
        /* This is not a clean close. If a finalize was called, we
         * would have deregistered this socket. The application might
         * have aborted. Just cleanup all the processes */
        status = HYD_PMCD_pmi_serv_cleanup();
        if (status != HYD_SUCCESS) {
            HYDU_Warn_printf("bootstrap server returned error cleaning up processes\n");
            status = HYD_SUCCESS;
            goto fn_fail;
        }

        status = HYD_DMX_deregister_fd(fd);
        if (status != HYD_SUCCESS) {
            HYDU_Warn_printf("unable to deregister fd %d\n", fd);
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
    cmd = strtok(buf, HYD_PMCD_pmi_handle->delim);
    for (i = 0; i < HYD_NUM_TMP_STRINGS; i++) {
        args[i] = strtok(NULL, HYD_PMCD_pmi_handle->delim);
        if (args[i] == NULL)
            break;
    }

    if (cmd == NULL) {
        status = HYD_SUCCESS;
    }
    else {
        /* Search for the PMI command in our table */
        status = HYDU_strsplit(cmd, &str1, &str2, '=');
        HYDU_ERR_POP(status, "string split returned error\n");

        h = HYD_PMCD_pmi_handle->handle_fns;
        while (h->handler) {
            if (!strcmp(str2, h->cmd)) {
                status = h->handler(fd, args);
                HYDU_ERR_POP(status, "PMI handler returned error\n");
                break;
            }
            h++;
        }
        if (!h->handler) {
            /* We don't understand the command */
            HYDU_Error_printf("Unrecognized PMI command: %s | cleaning up processes\n", cmd);

            /* Cleanup all the processes and return. We don't need to
             * check the return status since we are anyway returning
             * an error */
            HYD_PMCD_pmi_serv_cleanup();
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


HYD_Status HYD_PMCD_pmi_serv_control_connect_cb(int fd, HYD_Event_t events, void *userp)
{
    int accept_fd, partition_id, count;
    struct HYD_Partition *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a control socket connection */
    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    /* Read the partition ID */
    status = HYDU_sock_read(accept_fd, &partition_id, sizeof(int), &count,
                            HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "sock read returned error\n");

    /* Find the partition */
    FORALL_PARTITIONS(partition, handle.partition_list) {
        if (partition->base->partition_id == partition_id)
            break;
    }
    HYDU_ERR_CHKANDJUMP1(status, partition == NULL, HYD_INTERNAL_ERROR,
                         "cannot find partition with ID %d\n", partition_id);

    /* This will be the control socket for this partition */
    partition->control_fd = accept_fd;

    /* Send out the executable information */
    status = HYD_PMCD_pmi_send_exec_info(partition);
    HYDU_ERR_POP(status, "unable to send exec info to proxy\n");

    status = HYD_DMX_register_fd(1, &accept_fd, HYD_STDOUT, partition,
                                 HYD_PMCD_pmi_serv_control_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_serv_control_cb(int fd, HYD_Event_t events, void *userp)
{
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    int count, proc_count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    partition = (struct HYD_Partition *) userp;

    proc_count = 0;
    for (exec = partition->exec_list; exec; exec = exec->next)
        proc_count += exec->proc_count;

    HYDU_MALLOC(partition->exit_status, int *, proc_count * sizeof(int), status);

    status = HYDU_sock_read(fd, (void *) partition->exit_status, proc_count * sizeof(int),
                            &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read status from proxy\n");

    status = HYD_DMX_deregister_fd(fd);
    HYDU_ERR_POP(status, "error deregistering fd\n");

    close(fd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_serv_cleanup(void)
{
    struct HYD_Partition *partition;
    enum HYD_PMCD_pmi_proxy_cmds cmd;
    HYD_Status status = HYD_SUCCESS, overall_status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Instead of doing this from this process itself, fork a
     * bunch of processes to do this. */
    /* Connect to all proxies and send a KILL command */
    FORALL_ACTIVE_PARTITIONS(partition, handle.partition_list) {
        cmd = KILL_JOB;
        status = HYDU_sock_trywrite(partition->control_fd, &cmd,
                                    sizeof(enum HYD_PMCD_pmi_proxy_cmds));
        if (status != HYD_SUCCESS) {
            HYDU_Warn_printf("unable to send data to the proxy on %s\n",
                             partition->base->name);
            overall_status = HYD_INTERNAL_ERROR;
            continue;   /* Move on to the next proxy */
        }
    }

    HYDU_FUNC_EXIT();

    return overall_status;
}


void HYD_PMCD_pmi_serv_signal_cb(int signal)
{
    HYDU_FUNC_ENTER();

    if (signal == SIGINT || signal == SIGQUIT || signal == SIGTERM
#if defined SIGSTOP
        || signal == SIGSTOP
#endif /* SIGSTOP */
#if defined SIGCONT
        || signal == SIGCONT
#endif /* SIGSTOP */
) {
        /* There's nothing we can do with the return value for now. */
        HYD_PMCD_pmi_serv_cleanup();
        exit(-1);
    }
    else {
        /* Ignore other signals for now */
    }

    HYDU_FUNC_EXIT();
    return;
}
