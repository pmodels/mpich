/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmip.h"
#include "pmip_pmi.h"
#include "ckpoint.h"
#include "demux.h"
#include "topo.h"
#include "bsci.h"

static HYD_status control_cb(int fd, HYD_event_t events, void *userp);
HYD_status HYD_pmci_launch_child_procs(char *nodes);

struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_handle = { 0 };

static int pmi_storage_len = 0;
static char pmi_storage[HYD_TMPBUF_SIZE], *sptr = pmi_storage, r[HYD_TMPBUF_SIZE];
static int using_pmi_port = 0;

extern struct HYD_pmi_ranks2fds *r2f_table;

static HYD_status stdoe_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, i, sent, recvd, stdfd;
    char buf[HYD_TMPBUF_SIZE];
    struct HYD_pmcd_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    stdfd = (int) (size_t) userp;

    status = HYDU_sock_read(fd, buf, HYD_TMPBUF_SIZE, &recvd, &closed, HYDU_SOCK_COMM_NONE);
    HYDU_ERR_POP(status, "sock read error\n");

    if (recvd) {
        if (stdfd == STDOUT_FILENO) {
            HYD_pmcd_init_header(&hdr);
            hdr.cmd = STDOUT;
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.out[i] == fd)
                    break;
        }
        else {
            HYD_pmcd_init_header(&hdr);
            hdr.cmd = STDERR;
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.err[i] == fd)
                    break;
        }

        HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

        hdr.pgid = HYD_pmcd_pmip.local.pgid;
        hdr.proxy_id = HYD_pmcd_pmip.local.id;
        hdr.rank = HYD_pmcd_pmip.downstream.pmi_rank[i];
        hdr.buflen = recvd;

        {
            int upstream_sock_closed;

            status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent,
                                     &upstream_sock_closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "sock write error\n");
            HYDU_ASSERT(!upstream_sock_closed, status);

            status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, recvd, &sent,
                                     &upstream_sock_closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "sock write error\n");
            HYDU_ASSERT(!upstream_sock_closed, status);
        }
    }

    if (closed) {
        /* The connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        if (stdfd == STDOUT_FILENO) {
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.out[i] == fd)
                    HYD_pmcd_pmip.downstream.out[i] = HYD_FD_CLOSED;
        }
        else {
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.err[i] == fd)
                    HYD_pmcd_pmip.downstream.err[i] = HYD_FD_CLOSED;
        }

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status check_pmi_cmd(char **buf, int *pmi_version, int *repeat)
{
    int full_command, buflen, cmdlen;
    char *bufptr, lenptr[7];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *repeat = 0;

    /* We need to read at least 6 bytes before we can decide if this
     * is PMI-1 or PMI-2 */
    if (pmi_storage_len < 6)
        goto fn_exit;

    /* FIXME: This should really be a "FIXME" for the client, since
     * there's not much we can do on the server side.
     *
     * We initialize to whatever PMI version we detect while reading
     * the PMI command, instead of relying on what the init command
     * gave us. This part of the code should not know anything about
     * PMI-1 vs. PMI-2. But the simple PMI client-side code in MPICH
     * is so hacked up, that commands can arrive out-of-order and this
     * is necessary. This was discussed in the group and we felt that
     * it is unsafe to change the order of the PMI command arrival in
     * the client code (even if we are really correcting it), since
     * other PMs might rely on the "incorrect order of commands".
     */

    /* Parse the string and if a full command is found, make sure that
     * bufptr points to the last byte of the command */
    full_command = 0;
    if (!strncmp(sptr, "cmd=", strlen("cmd=")) || !strncmp(sptr, "mcmd=", strlen("mcmd="))) {
        /* PMI-1 format command; read the rest of it */
        *pmi_version = 1;

        if (!strncmp(sptr, "cmd=", strlen("cmd="))) {
            /* A newline marks the end of the command */
            for (bufptr = sptr; bufptr < sptr + pmi_storage_len; bufptr++) {
                if (*bufptr == '\n') {
                    full_command = 1;
                    break;
                }
            }
        }
        else {  /* multi commands */
            for (bufptr = sptr; bufptr < sptr + pmi_storage_len - strlen("endcmd\n") + 1; bufptr++) {
                if (bufptr[0] == 'e' && bufptr[1] == 'n' && bufptr[2] == 'd' &&
                    bufptr[3] == 'c' && bufptr[4] == 'm' && bufptr[5] == 'd' && bufptr[6] == '\n') {
                    full_command = 1;
                    bufptr += strlen("endcmd\n") - 1;
                    break;
                }
            }
        }
    }
    else {
        *pmi_version = 2;

        /* We already made sure we had at least 6 bytes */
        memcpy(lenptr, sptr, 6);
        lenptr[6] = 0;
        cmdlen = atoi(lenptr);

        if (pmi_storage_len >= cmdlen + 6) {
            full_command = 1;
            bufptr = sptr + 6 + cmdlen - 1;
        }
    }

    if (full_command) {
        /* We have a full command */
        buflen = bufptr - sptr + 1;
        HYDU_MALLOC_OR_JUMP(*buf, char *, buflen, status);
        memcpy(*buf, sptr, buflen);
        sptr += buflen;
        pmi_storage_len -= buflen;
        (*buf)[buflen - 1] = '\0';

        if (pmi_storage_len == 0)
            sptr = pmi_storage;
        else
            *repeat = 1;
    }
    else {
        /* We don't have a full command. Copy the rest of the data to
         * the front of the storage buffer. */

        /* FIXME: This dual memcpy is crazy and needs to be
         * fixed. Single memcpy should be possible, but we need to be
         * a bit careful not to corrupt the buffer. */
        if (sptr != pmi_storage) {
            memcpy(r, sptr, pmi_storage_len);
            memcpy(pmi_storage, r, pmi_storage_len);
            sptr = pmi_storage;
        }
        *buf = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_cb(int fd, HYD_event_t events, void *userp)
{
    char *buf = NULL, *pmi_cmd = NULL, **args = NULL;
    int closed, repeat, sent, i = -1, linelen, pid = -1;
    struct HYD_pmcd_hdr hdr;
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    if(HYD_pmcd_pmip.user_global.branch_count != -1) {
        hdr.pid = HYD_pmcd_pmip.local.id;
    } else {
        hdr.pid = fd;
    }

    /* Try to find the PMI FD */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pmi_fd[i] == fd) {
            pid = i;
            break;
        }
    }

  read_cmd:
    /* PMI-1 does not tell us how much to read. We read how much ever
     * we can, parse out full PMI commands from it, and process
     * them. When we don't have a full PMI command, we go back and
     * read from the same FD until we do. PMI clients (1 and 2) always
     * send full commands, then wait for response. */
    status =
        HYDU_sock_read(fd, pmi_storage + pmi_storage_len, HYD_TMPBUF_SIZE - pmi_storage_len,
                       &linelen, &closed, HYDU_SOCK_COMM_NONE);
    HYDU_ERR_POP(status, "unable to read PMI command\n");

    if (closed) {
        /* If a PMI application terminates, we clean up the remaining
         * processes. For a correct PMI application, we should never
         * get closed socket event as we deregister this socket as
         * soon as we get the finalize message. For non-PMI
         * applications, this is harder to identify, so we just let
         * the user cleanup the processes on a failure.
         *
         * We check of we found the PMI FD, and if the FD is "PMI
         * active" (which means that this is an MPI application).
         */
        if (pid != -1 && HYD_pmcd_pmip.downstream.pmi_fd_active[pid]) {
            /* If this is not a forced cleanup, store a temporary
             * erroneous exit status. In case the application does not
             * return a non-zero exit status, we will use this. */
            if (HYD_pmcd_pmip.downstream.forced_cleanup == 0)
                HYD_pmcd_pmip.downstream.exit_status[pid] = 1;

            /* Deregister failed socket */
            status = HYDT_dmx_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to deregister fd\n");
            close(fd);

            if (HYD_pmcd_pmip.user_global.auto_cleanup) {
                /* kill all processes */
                HYD_pmcd_pmip_send_signal(SIGKILL);
            }
            else {
                /* If the user doesn't want to automatically cleanup,
                 * signal the remaining processes, and send this
                 * information upstream */

                /* FIXME: This code needs to change from sending the
                 * SIGUSR1 signal to a PMI-2 notification message. */
                HYD_pmcd_pmip_send_signal(SIGUSR1);

                hdr.cmd = PROCESS_TERMINATED;
                hdr.pid = HYD_pmcd_pmip.downstream.pmi_rank[pid];
                hdr.buflen = 0;
                status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr),
                                         &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
                HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
                HYDU_ASSERT(!closed, status);
            }
        }
        goto fn_exit;
    }
    else {
        pmi_storage_len += linelen;
        pmi_storage[pmi_storage_len] = 0;
    }

  check_cmd:
    status = check_pmi_cmd(&buf, &hdr.pmi_version, &repeat);
    HYDU_ERR_POP(status, "error checking the PMI command\n");

    if (buf == NULL)
        /* read more to get a full command. */
        goto read_cmd;

    /* We were able to read the PMI command correctly. If we were able
     * to identify what PMI FD this is, activate it. If we were not
     * able to identify the PMI FD, we will activate it when we get
     * the PMI initialization command. */
    if (pid != -1 && !HYD_pmcd_pmip.downstream.pmi_fd_active[pid])
        HYD_pmcd_pmip.downstream.pmi_fd_active[pid] = 1;

    if (hdr.pmi_version == 1)
        HYD_pmcd_pmip_pmi_handle = HYD_pmcd_pmip_pmi_v1;
    else
        HYD_pmcd_pmip_pmi_handle = HYD_pmcd_pmip_pmi_v2;

    HYDU_MALLOC_OR_JUMP(args, char **, MAX_PMI_ARGS * sizeof(char *), status);
    for (i = 0; i < MAX_PMI_ARGS; i++)
        args[i] = NULL;

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "got pmi command (from %d): %s\n", fd, pmi_cmd);
        HYDU_print_strlist(args);
    }
    hdr.buflen = strlen(buf);
    if (using_pmi_port) {
        if (!strcmp(pmi_cmd, "initack")){
            /* Get rank from initack PMI request (buf is 'cmd=initack pmiid=<rank>') */
            hdr.rank = atoi(buf+18);
            HYDU_add_pmi_r2f(&r2f_table, hdr.rank, fd);
        } else {
            /* Get rank from r2f table */
            hdr.rank = HYDU_get_rank_by_fd(&r2f_table, fd);
        }
    } else {
        /* Get rank from r2f table */
        hdr.rank = HYDU_get_rank_by_fd(&r2f_table, fd);
    }

    h = HYD_pmcd_pmip_pmi_handle;
    hdr.cmd = PMI_CMD;
    while (h->handler) {
        if (!strcmp(pmi_cmd, h->cmd)) {
            status = h->handler(fd, args, &hdr);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand this command %s; forwarding upstream\n", pmi_cmd);
    }

    /* We don't understand the command; forward it upstream */
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
    HYDU_ASSERT(!closed, status);

    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
    HYDU_ASSERT(!closed, status);

    if (repeat)
        /* there are more commands to process. */
        goto check_cmd;

  fn_exit:
    if (pmi_cmd)
        MPL_free(pmi_cmd);
    if (args) {
        HYDU_free_strlist(args);
        MPL_free(args);
    }
    if (buf)
        MPL_free(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_pmi_response(int fd, struct HYD_pmcd_hdr hdr)
{
    int count, closed, sent, i;
    char *buf = NULL, *pmi_cmd = NULL, **args = NULL;
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen + 1, status);

    status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI response from proxy\n");
    HYDU_ASSERT(!closed, status);

    buf[hdr.buflen] = 0;

    HYDU_MALLOC_OR_JUMP(args, char **, MAX_PMI_INTERNAL_ARGS * sizeof(char *), status);
    for (i = 0; i < MAX_PMI_INTERNAL_ARGS; i++)
        args[i] = NULL;

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

    h = HYD_pmcd_pmip_pmi_handle;
    while (h->handler) {
        if (!strcmp(pmi_cmd, h->cmd)) {
            status = h->handler(fd, args, &hdr);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand the response %s; forwarding downstream\n", pmi_cmd);
    }

    if (HYD_pmcd_pmip.user_global.branch_count != -1 && hdr.pid != HYD_pmcd_pmip.local.id) {
        status = HYDU_sock_write(HYD_pmcd_pmip.pid_to_fd[hdr.pid], &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to sent PMI_RESPONSE header to proxy\n");
        status = HYDU_sock_write(HYD_pmcd_pmip.pid_to_fd[hdr.pid], buf, hdr.buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    } else {
        status = HYDU_sock_write(HYDU_get_fd_by_rank(&r2f_table, hdr.rank), buf, hdr.buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    }
    HYDU_ERR_POP(status, "unable to forward PMI response to MPI process\n");

    if (HYD_pmcd_pmip.user_global.auto_cleanup) {
        HYDU_ASSERT(!closed, status);
    }
    else {
        /* Ignore the error and drop the PMI response */
    }

  fn_exit:
    if (pmi_cmd)
        MPL_free(pmi_cmd);
    if (args) {
        HYDU_free_strlist(args);
        MPL_free(args);
    }
    if (buf)
        MPL_free(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_pmi_command(int fd, struct HYD_pmcd_hdr hdr)
{
    int count, closed, sent;
    char *buf = NULL, *pmi_cmd = NULL, *args[MAX_PMI_INTERNAL_ARGS] = { 0 };
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen + 1, status);

    status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI command from proxy\n");
    HYDU_ASSERT(!closed, status);

    buf[hdr.buflen] = 0;

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

    h = HYD_pmcd_pmip_pmi_handle;
    while (h->handler) {
        if (!strcmp(pmi_cmd, h->cmd)) {
            status = h->handler(fd, args, &hdr);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand the %s; forwarding upstream\n", pmi_cmd);
    }

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to forward PMI command upstream\n");

    if (HYD_pmcd_pmip.user_global.auto_cleanup) {
        HYDU_ASSERT(!closed, status);
    }
    else {
        /* Ignore the error and drop the PMI response */
    }

  fn_exit:
    if (pmi_cmd)
        MPL_free(pmi_cmd);
    HYDU_free_strlist(args);
    if (buf)
        MPL_free(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_launch_children(int fd, struct HYD_pmcd_hdr *hdr)
{
    int count, closed;
    char *buf=NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(buf, char *, hdr->buflen + 1, status);
    status = HYDU_sock_read(fd, buf, hdr->buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI response from proxy\n");
    HYDU_ASSERT(!closed, status);

    buf[hdr->buflen] = 0;
    status = HYD_pmci_launch_child_procs(buf);
    HYDU_ERR_POP(status, "unable to launch child proxy\n");

  fn_exit:
    if (buf)
        MPL_free(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_status pmi_listen_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    status = HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, userp, pmi_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static int local_to_global_id(int local_id)
{
    int rem1, rem2, layer, ret;

    if (local_id < HYD_pmcd_pmip.system_global.global_core_map.local_filler)
        ret = HYD_pmcd_pmip.system_global.pmi_id_map.filler_start + local_id;
    else {
        rem1 = local_id - HYD_pmcd_pmip.system_global.global_core_map.local_filler;
        layer = rem1 / HYD_pmcd_pmip.system_global.global_core_map.local_count;
        rem2 = rem1 - (layer * HYD_pmcd_pmip.system_global.global_core_map.local_count);

        ret = HYD_pmcd_pmip.system_global.pmi_id_map.non_filler_start +
            (layer * HYD_pmcd_pmip.system_global.global_core_map.global_count) + rem2;
    }

    return ret;
}

static HYD_status launch_procs(void)
{
    int i, j, process_id, dummy;
    char *str, *envstr, *list, *pmi_port;
    struct HYD_string_stash stash;
    struct HYD_env *env, *force_env = NULL;
    struct HYD_exec *exec;
    struct HYD_pmcd_hdr hdr;
    int sent, closed, pmi_fds[2] = { HYD_FD_UNSET, HYD_FD_UNSET };
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_pmip.local.proxy_process_count = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next)
        HYD_pmcd_pmip.local.proxy_process_count += exec->proc_count;

    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.out, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.err, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pid, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.exit_status, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pmi_rank, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pmi_fd, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pmi_fd_active, int *,
                        HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);

    /* Initialize the PMI_FD and PMI FD active state, and exit status */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        /* The exit status is populated when the processes terminate */
        HYD_pmcd_pmip.downstream.exit_status[i] = -1;

        /* If we use PMI_FD, the pmi_fd and pmi_fd_active arrays will
         * be filled out in this function. But if we are using
         * PMI_PORT, we will fill them out later when the processes
         * send the PMI initialization message. Note that non-MPI
         * processes are never "PMI active" when we use the PMI
         * PORT. */
        HYD_pmcd_pmip.downstream.pmi_fd[i] = HYD_FD_UNSET;
        HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 0;
        HYD_pmcd_pmip.downstream.pmi_rank[i] = local_to_global_id(i);
    }

    status = HYDT_topo_init(HYD_pmcd_pmip.user_global.topolib,
                            HYD_pmcd_pmip.user_global.binding,
                            HYD_pmcd_pmip.user_global.mapping, HYD_pmcd_pmip.user_global.membind);
    HYDU_ERR_POP(status, "unable to initialize process topology\n");

    status = HYDT_ckpoint_init(HYD_pmcd_pmip.user_global.ckpointlib,
                               HYD_pmcd_pmip.user_global.ckpoint_num);
    HYDU_ERR_POP(status, "unable to initialize checkpointing\n");

    if (HYD_pmcd_pmip.user_global.ckpoint_prefix) {
        using_pmi_port = 1;
        status = HYDU_sock_create_and_listen_portstr(HYD_pmcd_pmip.user_global.iface,
                                                     NULL, NULL, &pmi_port, pmi_listen_cb, NULL);
        HYDU_ERR_POP(status, "unable to create PMI port\n");
    }

    if (HYD_pmcd_pmip.exec_list->exec[0] == NULL) {     /* Checkpoint restart case */
        status = HYDU_env_create(&env, "PMI_PORT", pmi_port);
        HYDU_ERR_POP(status, "unable to create env\n");

        /* Restart the proxy -- we use the first prefix in the list */
        status = HYDT_ckpoint_restart(HYD_pmcd_pmip.local.pgid, HYD_pmcd_pmip.local.id,
                                      env, HYD_pmcd_pmip.local.proxy_process_count,
                                      HYD_pmcd_pmip.downstream.pmi_rank,
                                      HYD_pmcd_pmip.downstream.pmi_rank[0] ? NULL :
                                      &HYD_pmcd_pmip.downstream.in,
                                      HYD_pmcd_pmip.downstream.out,
                                      HYD_pmcd_pmip.downstream.err,
                                      HYD_pmcd_pmip.downstream.pid,
                                      HYD_pmcd_pmip.local.ckpoint_prefix_list[0]);
        HYDU_ERR_POP(status, "unable to restart from checkpoint\n");

        goto fn_spawn_complete;
    }

    /* Spawn the processes */
    process_id = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {

        /* Increasing priority order: (1) global inherited env; (2)
         * global user env; (3) local user env; (4) system env. We
         * just set them one after the other, overwriting the previous
         * written value if needed. */

        if (!exec->env_prop && HYD_pmcd_pmip.user_global.global_env.prop)
            exec->env_prop = MPL_strdup(HYD_pmcd_pmip.user_global.global_env.prop);

        if (!exec->env_prop) {
            /* user didn't specify anything; add inherited env to optional env */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (!strcmp(exec->env_prop, "all")) {
            /* user explicitly asked us to pass all the environment */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (!strncmp(exec->env_prop, "list", strlen("list"))) {
            if (exec->env_prop)
                list = MPL_strdup(exec->env_prop + strlen("list:"));
            else
                list = MPL_strdup(HYD_pmcd_pmip.user_global.global_env.prop + strlen("list:"));

            envstr = strtok(list, ",");
            while (envstr) {
                env = HYDU_env_lookup(envstr, HYD_pmcd_pmip.user_global.global_env.inherited);
                if (env) {
                    status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                }
                envstr = strtok(NULL, ",");
            }
        }

        /* global user env */
        for (env = HYD_pmcd_pmip.user_global.global_env.user; env; env = env->next) {
            status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* local user env */
        for (env = exec->user_env; env; env = env->next) {
            status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* system env */
        for (env = HYD_pmcd_pmip.user_global.global_env.system; env; env = env->next) {
            status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* Set the interface hostname based on what the user provided */
        if (HYD_pmcd_pmip.local.iface_ip_env_name) {
            if (HYD_pmcd_pmip.user_global.iface) {
                char *ip;

                status = HYDU_sock_get_iface_ip(HYD_pmcd_pmip.user_global.iface, &ip);
                HYDU_ERR_POP(status, "unable to get IP address for %s\n",
                             HYD_pmcd_pmip.user_global.iface);

                /* The user asked us to use a specific interface; let's find it */
                status = HYDU_append_env_to_list(HYD_pmcd_pmip.local.iface_ip_env_name,
                                                 ip, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
            else if (HYD_pmcd_pmip.local.hostname) {
                /* The second choice is the hostname the user gave */
                status = HYDU_append_env_to_list(HYD_pmcd_pmip.local.iface_ip_env_name,
                                                 HYD_pmcd_pmip.local.hostname, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }

        if (exec->wdir && chdir(exec->wdir) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "unable to change wdir to %s (%s)\n", exec->wdir,
                                MPL_strerror(errno));

        for (i = 0; i < exec->proc_count; i++) {
            /* FIXME: these envvars should be set by MPICH instead. See #2360 */
            str = HYDU_int_to_str(HYD_pmcd_pmip.local.proxy_process_count);
            status = HYDU_append_env_to_list("MPI_LOCALNRANKS", str, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");

            str = HYDU_int_to_str(process_id);
            status = HYDU_append_env_to_list("MPI_LOCALRANKID", str, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");

            if (using_pmi_port) {
                /* If we are using the PMI_PORT format */

                /* PMI_PORT */
                status = HYDU_append_env_to_list("PMI_PORT", pmi_port, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                /* PMI_ID */
                str = HYDU_int_to_str(HYD_pmcd_pmip.downstream.pmi_rank[process_id]);
                status = HYDU_append_env_to_list("PMI_ID", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);
            }
            else {
                /* PMI_RANK */
                str = HYDU_int_to_str(HYD_pmcd_pmip.downstream.pmi_rank[process_id]);
                status = HYDU_append_env_to_list("PMI_RANK", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);

                if (socketpair(AF_UNIX, SOCK_STREAM, 0, pmi_fds) < 0)
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

                status = HYDT_dmx_register_fd(1, &pmi_fds[0], HYD_POLLIN, NULL, pmi_cb);
                HYDU_ERR_POP(status, "unable to register fd\n");

                status = HYDU_sock_cloexec(pmi_fds[0]);
                HYDU_ERR_POP(status, "unable to set socket to close on exec\n");

                HYD_pmcd_pmip.downstream.pmi_fd[process_id] = pmi_fds[0];
                str = HYDU_int_to_str(pmi_fds[1]);

                status = HYDU_append_env_to_list("PMI_FD", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);

                /* PMI_SIZE */
                str = HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_process_count);
                status = HYDU_append_env_to_list("PMI_SIZE", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);

                HYDU_add_pmi_r2f(&r2f_table, HYD_pmcd_pmip.downstream.pmi_rank[process_id], pmi_fds[0]);
            }

            HYD_STRING_STASH_INIT(stash);
            for (j = 0; exec->exec[j]; j++)
                HYD_STRING_STASH(stash, MPL_strdup(exec->exec[j]), status);

            /* For non rank-0 processes, store the stdin socket in a
             * dummy variable instead of passing NULL.  Passing NULL
             * will cause the create_process function to close the
             * STDIN socket, allowing the process to reuse that
             * socket.  However, if an application reopens stdin, it
             * causes an incorrect socket (which is not STDIN) to be
             * closed.  This is technically a user application bug,
             * but this is a safe-guard to workaround that.  See
             * ticket #1622 for more details. */
            status = HYDU_create_process(stash.strlist, force_env,
                                         HYD_pmcd_pmip.downstream.pmi_rank[process_id] ? &dummy :
                                         &HYD_pmcd_pmip.downstream.in,
                                         &HYD_pmcd_pmip.downstream.out[process_id],
                                         &HYD_pmcd_pmip.downstream.err[process_id],
                                         &HYD_pmcd_pmip.downstream.pid[process_id], process_id);
            HYDU_ERR_POP(status, "create process returned error\n");

            if (HYD_pmcd_pmip.downstream.in != HYD_FD_UNSET) {
                status = HYDU_sock_set_nonblock(HYD_pmcd_pmip.downstream.in);
                HYDU_ERR_POP(status, "unable to set stdin socket to non-blocking\n");
            }

            HYD_STRING_STASH_FREE(stash);

            if (pmi_fds[1] != HYD_FD_UNSET) {
                close(pmi_fds[1]);
                pmi_fds[1] = HYD_FD_CLOSED;
            }

            process_id++;
        }

        HYDU_env_free_list(force_env);
        force_env = NULL;
    }

    /* Send the PID list upstream */
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PID_LIST;
    if (HYD_pmcd_pmip.user_global.branch_count != -1) {
        hdr.pid = HYD_pmcd_pmip.local.id;
        hdr.pmi_version = 1;
        hdr.buflen = HYD_pmcd_pmip.local.proxy_process_count * sizeof(int);
        hdr.rank = 0;
    }
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.pid,
                             HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), &sent,
                             &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PID list upstream\n");
    HYDU_ASSERT(!closed, status);

  fn_spawn_complete:
    /* Everything is spawned, register the required FDs  */
    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.out, HYD_POLLIN,
                                  (void *) (size_t) STDOUT_FILENO, stdoe_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.err, HYD_POLLIN,
                                  (void *) (size_t) STDERR_FILENO, stdoe_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status parse_exec_params(char **t_argv)
{
    char **argv = t_argv;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
        /* Get the executable arguments  */
        status = HYDU_parse_array(&argv, HYD_pmcd_pmip_match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        /* No more arguments left */
        if (!(*argv))
            break;
    } while (1);

    /* verify the arguments we got */
    if (HYD_pmcd_pmip.system_global.global_core_map.local_filler == -1 ||
        HYD_pmcd_pmip.system_global.global_core_map.local_count == -1 ||
        HYD_pmcd_pmip.system_global.global_core_map.global_count == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "cannot find global core map (%d,%d,%d)\n",
                            HYD_pmcd_pmip.system_global.global_core_map.local_filler,
                            HYD_pmcd_pmip.system_global.global_core_map.local_count,
                            HYD_pmcd_pmip.system_global.global_core_map.global_count);

    if (HYD_pmcd_pmip.system_global.pmi_id_map.filler_start == -1 ||
        HYD_pmcd_pmip.system_global.pmi_id_map.non_filler_start == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "cannot find pmi id map (%d,%d)\n",
                            HYD_pmcd_pmip.system_global.pmi_id_map.filler_start,
                            HYD_pmcd_pmip.system_global.pmi_id_map.non_filler_start);

    if (HYD_pmcd_pmip.local.proxy_core_count == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "proxy core count not available\n");

    if (HYD_pmcd_pmip.exec_list == NULL && HYD_pmcd_pmip.user_global.ckpoint_prefix == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "no executable given and doesn't look like a restart either\n");

    /* Set default values */
    if (HYD_pmcd_pmip.user_global.topolib == NULL && HYDRA_DEFAULT_TOPOLIB)
        HYD_pmcd_pmip.user_global.topolib = MPL_strdup(HYDRA_DEFAULT_TOPOLIB);

#ifdef HYDRA_DEFAULT_CKPOINTLIB
    if (HYD_pmcd_pmip.user_global.ckpointlib == NULL)
        HYD_pmcd_pmip.user_global.ckpointlib = MPL_strdup(HYDRA_DEFAULT_CKPOINTLIB);
#endif

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status procinfo(int fd)
{
    char **arglist;
    int num_strings, str_len, recvd, i, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Read information about the application to launch into a string
     * array and call parse_exec_params() to interpret it and load it into
     * the proxy handle. */
    status = HYDU_sock_read(fd, &num_strings, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data from upstream\n");
    HYDU_ASSERT(!closed, status);

    HYDU_MALLOC_OR_JUMP(arglist, char **, (num_strings + 1) * sizeof(char *), status);

    for (i = 0; i < num_strings; i++) {
        status = HYDU_sock_read(fd, &str_len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC_OR_JUMP(arglist[i], char *, str_len, status);

        status = HYDU_sock_read(fd, arglist[i], str_len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);
    }
    arglist[num_strings] = NULL;

    /* Get the parser to fill in the proxy params structure. */
    status = parse_exec_params(arglist);
    HYDU_ERR_POP(status, "unable to parse argument list\n");

    HYDU_free_strlist(arglist);
    MPL_free(arglist);

    /* Save this fd as we need to send back the exit status on
     * this. */
    HYD_pmcd_pmip.upstream.control = fd;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status send_procinfo_downstream(int in_fd, int out_fd, int proxy_id)
{
    struct HYD_pmcd_hdr hdr;
    char *string = NULL;
    int num_strings, str_len, recvd, i, curr_alloc = 0;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PROC_INFO;
    status = HYDU_sock_write(out_fd, &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(out_fd, &proxy_id, sizeof(int), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to write data to proxy\n");
    HYDU_ASSERT(!closed, status);

    /* Read information about the application to launch into a string
     * the proxy handle. */
    status = HYDU_sock_read(in_fd, &num_strings, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data from upstream\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(out_fd, &num_strings, sizeof(int), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error writing data to downstream\n");
    HYDU_ASSERT(!closed, status);

    for (i = 0; i < num_strings; i++) {
        status = HYDU_sock_read(in_fd, &str_len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);
        status = HYDU_sock_write(out_fd, &str_len, sizeof(int), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error writing data to downstream\n");
        HYDU_ASSERT(!closed, status);

        if (curr_alloc < str_len) {
            if (string)
                MPL_free(string);
               HYDU_MALLOC_OR_JUMP(string, char *, str_len, status);
            curr_alloc = str_len;
        }

        status = HYDU_sock_read(in_fd, string, str_len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);
        status = HYDU_sock_write(out_fd, string, str_len, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error writing data to downstream\n");
        HYDU_ASSERT(!closed, status);
    }


  fn_exit:
    if(string)
        MPL_free(string);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    int cmd_len, closed;
    struct HYD_pmcd_hdr hdr;
    char *buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a command from upstream */
    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &cmd_len, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading command from launcher\n");
    HYDU_ASSERT(!closed, status);

    if (hdr.cmd == PROC_INFO) {
        if(HYD_pmcd_pmip.user_global.branch_count != -1) {
            int proxy_id;

            status = HYDU_sock_read(fd, &proxy_id, sizeof(int), &cmd_len, &closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "error reading command from launcher\n");
            HYDU_ASSERT(!closed, status);
            if(proxy_id != HYD_pmcd_pmip.local.id) {
                /* transfer to child */
                status = send_procinfo_downstream(fd, HYD_pmcd_pmip.pid_to_fd[proxy_id], proxy_id);
                HYDU_ERR_POP(status, "error sending procinfo downstream\n");
                goto fn_exit;
            }
        }

        status = procinfo(fd);
        HYDU_ERR_POP(status, "error parsing process info\n");

        status = launch_procs();
        HYDU_ERR_POP(status, "launch_procs returned error\n");
    }
    else if (hdr.cmd == CKPOINT) {
        HYDU_dump(stdout, "requesting checkpoint\n");

        status = HYDT_ckpoint_checkpoint(HYD_pmcd_pmip.local.pgid, HYD_pmcd_pmip.local.id,
                                         HYD_pmcd_pmip.local.ckpoint_prefix_list[0]);

        HYDU_ERR_POP(status, "checkpoint suspend failed\n");
        HYDU_dump(stdout, "checkpoint completed\n");
    }
    else if (hdr.cmd == PMI_RESPONSE) {
        status = handle_pmi_response(fd, hdr);
        HYDU_ERR_POP(status, "unable to handle PMI response\n");
    }
    else if (hdr.cmd == SIGNAL) {
        /* FIXME: This code needs to change from sending the signal to
         * a PMI-2 notification message. */
        HYD_pmcd_pmip_send_signal(hdr.signum);
    }
    else if (hdr.cmd == STDIN) {
        int count;

        if (hdr.buflen) {
            HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen, status);
            HYDU_ERR_POP(status, "unable to allocate memory\n");

            status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "unable to read from control socket\n");
            HYDU_ASSERT(!closed, status);

            if (HYD_pmcd_pmip.downstream.in == HYD_FD_CLOSED) {
                MPL_free(buf);
                goto fn_exit;
            }

            status = HYDU_sock_write(HYD_pmcd_pmip.downstream.in, buf, hdr.buflen, &count,
                                     &closed, HYDU_SOCK_COMM_NONE);
            HYDU_ERR_POP(status, "unable to write to downstream stdin\n");

            HYDU_ERR_CHKANDJUMP(status, count != hdr.buflen, HYD_INTERNAL_ERROR,
                                "process reading stdin too slowly; can't keep up\n");

            HYDU_ASSERT(count == hdr.buflen, status);

            if (HYD_pmcd_pmip.user_global.auto_cleanup) {
                HYDU_ASSERT(!closed, status);
            }
            else if (closed) {
                close(HYD_pmcd_pmip.downstream.in);
                HYD_pmcd_pmip.downstream.in = HYD_FD_CLOSED;
            }

            MPL_free(buf);
        }
        else {
            close(HYD_pmcd_pmip.downstream.in);
            HYD_pmcd_pmip.downstream.in = HYD_FD_CLOSED;
        }
    }
    else if ( hdr.cmd == LAUNCH_CHILD_PROXY ) {
        status = handle_launch_children(fd, &hdr);
        HYDU_ERR_POP(status, "unable to handle launch children\n");
    }
    else {
        status = HYD_INTERNAL_ERROR;
    }

    HYDU_ERR_POP(status, "error handling proxy command\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status HYD_pmcd_pmiserv_control_listen_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd, proxy_id, count, pgid, closed;
    HYD_status status = HYD_SUCCESS;
    int i;

    HYDU_FUNC_ENTER();

    /* We got a control socket connection */
    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    /* Get the PGID of the connection */
    pgid = ((int) (size_t) userp);

    /* Read the proxy ID */
    status = HYDU_sock_read(accept_fd, &proxy_id, sizeof(int), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "sock read returned error\n");
    HYDU_ASSERT(!closed, status);

    /* Find the proxy */
    for (i = 0; i < HYD_pmcd_pmip.children.num; i++) {
        if (HYD_pmcd_pmip.children.proxy_id[i] == proxy_id) {
            break;
        }
    }
    if (i == HYD_pmcd_pmip.children.num)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                             "cannot find proxy with ID %d\n", proxy_id);

    /* This will be the control socket for this proxy */
    HYD_pmcd_pmip.children.proxy_fd[i] = accept_fd;

    /* Ask to start children if necessary */
    if(HYD_pmcd_pmip.nodes_lists != NULL && HYD_pmcd_pmip.nodes_lists[i] != NULL) {
        HYDU_send_start_children(accept_fd, HYD_pmcd_pmip.nodes_lists[i], proxy_id);
    }

    status = HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, &i, control_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    HYD_pmcd_pmip.pid_to_fd[proxy_id] = accept_fd;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmci_launch_child_procs(char *nodes)
{
    struct HYD_node *node_list = NULL, *node, *tnode;
    struct HYD_string_stash proxy_stash;
    char *control_port = NULL;
    char *pmi_port = NULL;
    int pmi_id = -1, ret;
    HYD_status status = HYD_SUCCESS;
    char *node_point;
    int num_nodes = 0, i, size, num_child;
    int max_nodeid = 0;
    int enable_stdin;
    int *control_fd;
    HYDU_FUNC_ENTER();

    /* Initialize PMI */
    ret = MPL_env2str("PMI_PORT", (const char **) &pmi_port);
    if (ret) {  /* PMI_PORT already set */
        pmi_port = MPL_strdup(pmi_port);

        ret = MPL_env2int("PMI_ID", &pmi_id);
        if (!ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI_PORT set but not PMI_ID\n");
    }
    else {
        pmi_id = -1;
    }
    if (HYD_pmcd_pmip.user_global.debug == 1)
        HYDU_dump(stdout, "PMI port: %s; PMI ID: %d\n", pmi_port, pmi_id);

    /* Copy the host list to pass to the bootstrap server */
    node_list = NULL;
    tnode = NULL;
    node_point = strtok(nodes, ",");
    for (num_nodes = 0, i = 0; node_point; ) {
        HYDU_alloc_node(&node);
        node->hostname = MPL_strdup(node_point);
        node_point = strtok(NULL, ",");
        node->core_count = atoi(node_point);
        node_point = strtok(NULL, ",");
        node->node_id = atoi(node_point);
        node_point = strtok(NULL, ",");
        node->next = NULL;
        if(node->node_id > max_nodeid) {
            max_nodeid = node->node_id;
        }

        if (node_list == NULL) {
            node_list = node;
        } else {
            tnode->next = node;
        }
        tnode = node;
        num_nodes++;
    }


    status = HYDU_sock_create_and_listen_portstr(HYD_pmcd_pmip.user_global.iface,
                                                 NULL, NULL, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) 0);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_pmcd_pmip.user_global.debug == 1)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    status = HYD_pmcd_pmi_fill_in_child_proxy_args(&proxy_stash, control_port, 0, node_list->node_id);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

    if(num_nodes > HYD_pmcd_pmip.user_global.branch_count) {
        int i;
        node = node_list;
        for(i = 1; i < HYD_pmcd_pmip.user_global.branch_count; i++) {
            node = node->next;
        }
        tnode = node->next;
        node->next = NULL;
        num_child = HYD_pmcd_pmip.user_global.branch_count;
        i = (num_nodes - 1) / HYD_pmcd_pmip.user_global.branch_count;
        HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.nodes_lists, char **, HYD_pmcd_pmip.user_global.branch_count * sizeof (char *), status);
        memset(HYD_pmcd_pmip.nodes_lists, 0, HYD_pmcd_pmip.user_global.branch_count * sizeof (char *));
        node_list_to_strings(tnode, i, HYD_pmcd_pmip.nodes_lists);
    } else {
        num_child = num_nodes;
    }
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.children.exited, int *, num_child* sizeof (int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.children.proxy_id, int *, num_child * sizeof (int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.children.proxy_fd, int *, num_child * sizeof (int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.pid_to_fd, int *, (max_nodeid + 1) * sizeof (int), status);
    HYDU_MALLOC_OR_JUMP(control_fd, int *, num_child * sizeof(int), status);

    node = node_list;
    for(i = 0 ; i < num_child; i++) {
        HYD_pmcd_pmip.children.proxy_id[i] = node->node_id;
        HYD_pmcd_pmip.children.exited[i] = 0;
        node = node->next;
        control_fd[i] = HYD_FD_UNSET;
    }
    HYD_pmcd_pmip.children.num = num_child;

    status = HYDT_bsci_launch_procs(proxy_stash.strlist, NULL, node_list, HYD_FALSE, control_fd);
    HYDU_ERR_POP(status, "launcher server cannot launch processes\n");

  fn_exit:
    if (pmi_port)
        MPL_free(pmi_port);
    if (control_port)
        MPL_free(control_port);
    HYD_STRING_STASH_FREE(proxy_stash);
    HYDU_free_node_list(node_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status control_cb(int fd, HYD_event_t events, void *userp)
{
    char *buf = NULL, *pmi_cmd = NULL, *args[MAX_PMI_ARGS] = { 0 };
    int sent, closed, repeat;
    struct HYD_pmcd_hdr hdr;
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;
    int count;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI header from proxy\n");
    HYDU_ASSERT(!closed, status)

    /* Signal propagation */
    if (hdr.cmd == SIGNAL) {
        int i;

        /* Send signal upstream */
        status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to send SIGNAL command to proxy\n");
        HYDU_ASSERT(!closed, status);

        /* Send signal downstream except child proxy which sent this signal */
        for( i = 0; i < HYD_pmcd_pmip.children.num; i++ ) {
            if (fd == HYD_pmcd_pmip.children.proxy_fd[i])
                continue;

            status = HYDU_sock_write(HYD_pmcd_pmip.children.proxy_fd[i], &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "unable to send SIGNAL command to proxy\n");
            HYDU_ASSERT(!closed, status);
        }

        status = HYD_SUCCESS;
        goto fn_exit;
    }

    if (hdr.buflen != 0) {
        HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen + 1, status);
        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to read PMI command\n");
        HYDU_ASSERT(!closed, status);
        buf[hdr.buflen] = 0;
    }

    /* Handle exit_status */
    if(hdr.cmd == EXIT_STATUS) {
        int i;

        for(i = 0; i < HYD_pmcd_pmip.children.num; i++) {
            if(HYD_pmcd_pmip.children.proxy_id[i] == hdr.pid) {
                HYD_pmcd_pmip.children.exited[i] = 1;
                break;
            }
        }

        /* Check the hdr.rank, in this case the lower level lets us know to close this socket, */
        /* because lower level child proxies already has sent EXIT_STATUS */
        if (hdr.rank == 1) {
            status = HYDT_dmx_deregister_fd(fd);
            HYDU_ERR_POP(status, "error deregistering fd\n");
            close(fd);
        }
        hdr.rank = 0;
    } else {
        if (hdr.pid > 0) {
            HYD_pmcd_pmip.pid_to_fd[hdr.pid] = fd;
        }
    }


    /* Handle barrier_in */
    if ( hdr.cmd == PMI_CMD) {
        if (hdr.pmi_version == 1)
            h = HYD_pmcd_pmip_pmi_v1;
        else
            h = HYD_pmcd_pmip_pmi_v2;
        status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
        HYDU_ERR_POP(status, "unable to parse PMI command\n");

        if ( pmi_cmd && !strncmp(pmi_cmd, "barrier_in", 10) ) {
            while (h->handler) {
                if (!strcmp(pmi_cmd, h->cmd)) {
                    status = h->handler(fd, NULL, &hdr);
                    HYDU_ERR_POP(status, "PMI handler returned error\n");
                    goto fn_exit;
                }
                h++;
            }
        }
    }

    /* Forward PMI command upstream */
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
    HYDU_ASSERT(!closed, status);

    if (hdr.buflen != 0) {
        status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
        HYDU_ASSERT(!closed, status);
    }

  fn_exit:
    if (pmi_cmd)
        MPL_free(pmi_cmd);
    if (buf)
        MPL_free(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
