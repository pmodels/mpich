/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"
#include "pmip.h"
#include "pmip_pmi.h"
#include "demux.h"
#include "topo.h"

#define MAX_GPU_STR_LEN   (128)

struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_handle = { 0 };

static int pmi_storage_len = 0;
static char pmi_storage[HYD_TMPBUF_SIZE], *sptr = pmi_storage;

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
            hdr.cmd = CMD_STDOUT;
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.out[i] == fd)
                    break;
        } else {
            HYD_pmcd_init_header(&hdr);
            hdr.cmd = CMD_STDERR;
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.err[i] == fd)
                    break;
        }

        HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

        hdr.u.io.pgid = HYD_pmcd_pmip.local.pgid;
        hdr.u.io.proxy_id = HYD_pmcd_pmip.local.id;
        hdr.u.io.rank = HYD_pmcd_pmip.downstream.pmi_rank[i];
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
        } else {
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

static HYD_status handle_pmi_cmd(int fd, char *buf, int buflen, int pmi_version)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct HYD_pmcd_pmip_pmi_handle pmi_handle_fns[] = {
        {"init", fn_init},
        {"initack", fn_fullinit},
        {"get_maxes", fn_get_maxes},
        {"get_appnum", fn_get_appnum},
        {"get_my_kvsname", fn_get_my_kvsname},
        {"get_universe_size", fn_get_usize},
        {"get", fn_get},
        {"put", fn_put},
        {"barrier_in", fn_barrier_in},
        {"finalize", fn_finalize},
        {"fullinit", fn_fullinit},
        {"job-getid", fn_job_getid},
        {"info-putnodeattr", fn_info_putnodeattr},
        {"info-getnodeattr", fn_info_getnodeattr},
        {"info-getjobattr", fn_info_getjobattr},
        {"\0", NULL}
    };

    char *cmd = PMIU_wire_get_cmd(buf, buflen, pmi_version);

    struct HYD_pmcd_pmip_pmi_handle *h;
    h = pmi_handle_fns;
    while (h->handler) {
        if (strcmp(cmd, h->cmd) == 0) {
            struct PMIU_cmd pmi;
            status = PMIU_cmd_parse(buf, buflen, pmi_version, &pmi);
            HYDU_ERR_POP(status, "unable to parse PMI command\n");

            if (HYD_pmcd_pmip.user_global.debug) {
                HYDU_dump(stdout, "got pmi command\n    ");
                HYD_pmcd_pmi_dump(&pmi);
            }

            status = h->handler(fd, &pmi);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand this command, forwarding upstream\n");
        HYDU_dump(stdout, "    %s\n", buf);
    }

    /* We don't understand the command; forward it upstream */
    int sent, closed;

    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PMI;
    hdr.u.pmi.pmi_version = pmi_version;
    hdr.u.pmi.pid = fd;
    hdr.buflen = buflen;
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                             HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, buflen, &sent, &closed,
                             HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status check_pmi_cmd(char **buf, int *buflen_out, int *pmi_version, int *repeat)
{
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

    int full_command, buflen;
    /* Parse the string and if a full command is found, make sure that
     * buflen is the length of the buffer and NUL-terminated if necessary */
    full_command = 0;
    if (!strncmp(sptr, "cmd=", strlen("cmd=")) || !strncmp(sptr, "mcmd=", strlen("mcmd="))) {
        /* PMI-1 format command; read the rest of it */
        *pmi_version = 1;

        if (!strncmp(sptr, "cmd=", strlen("cmd="))) {
            /* A newline marks the end of the command */
            char *bufptr;
            for (bufptr = sptr; bufptr < sptr + pmi_storage_len; bufptr++) {
                if (*bufptr == '\n') {
                    full_command = 1;
                    *bufptr = '\0';
                    buflen = (int) (bufptr - sptr + 1);
                    break;
                }
            }
        } else {        /* multi commands */
            char *bufptr;
            for (bufptr = sptr; bufptr < sptr + pmi_storage_len - strlen("endcmd\n") + 1; bufptr++) {
                if (strncmp(bufptr, "endcmd\n", 7) == 0) {
                    full_command = 1;
                    bufptr += strlen("endcmd\n") - 1;
                    *bufptr = '\0';
                    buflen = (int) (bufptr - sptr + 1);
                    break;
                }
            }
        }
    } else {
        *pmi_version = 2;

        /* We already made sure we had at least 6 bytes */
        char lenptr[7];
        memcpy(lenptr, sptr, 6);
        lenptr[6] = 0;
        int cmdlen = atoi(lenptr);

        if (pmi_storage_len >= cmdlen + 6) {
            full_command = 1;
            char *bufptr = sptr + 6 + cmdlen - 1;
            *bufptr = '\0';
            buflen = (int) (bufptr - sptr + 1);
        }
    }

    if (full_command) {
        /* We have a full command */
        *buf = sptr;
        *buflen_out = buflen;
        sptr += buflen;
        pmi_storage_len -= buflen;

        if (pmi_storage_len == 0)
            sptr = pmi_storage;
        else
            *repeat = 1;
    } else {
        /* We don't have a full command. Copy the rest of the data to
         * the front of the storage buffer. */

        if (sptr != pmi_storage) {
            memmove(pmi_storage, sptr, pmi_storage_len);
            sptr = pmi_storage;
        }
        *buf = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
}

static HYD_status pmi_cb(int fd, HYD_event_t events, void *userp)
{
    char *buf = NULL;
    int closed, repeat, sent, linelen, pid = -1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Try to find the PMI FD */
    for (int i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
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
            /* Deregister failed socket */
            status = HYDT_dmx_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to deregister fd\n");
            close(fd);

            if (HYD_pmcd_pmip.user_global.auto_cleanup) {
                /* kill all processes */
                /* preset all exit_status except for the closed pid */
                for (int i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
                    if (i != pid &&
                        HYD_pmcd_pmip.downstream.exit_status[i] == PMIP_EXIT_STATUS_UNSET) {
                        HYD_pmcd_pmip.downstream.exit_status[i] = 0;
                    }
                }
                HYD_pmcd_pmip_send_signal(SIGKILL);
            } else {
                /* If the user doesn't want to automatically cleanup,
                 * signal the remaining processes, and send this
                 * information upstream */

                /* FIXME: This code needs to change from sending the
                 * SIGUSR1 signal to a PMI-2 notification message. */
                HYD_pmcd_pmip_send_signal(SIGUSR1);

                struct HYD_pmcd_hdr hdr;
                HYD_pmcd_init_header(&hdr);

                hdr.cmd = CMD_PROCESS_TERMINATED;
                /* global rank for the terminated process */
                hdr.u.data = HYD_pmcd_pmip.downstream.pmi_rank[pid];
                status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr),
                                         &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
                HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
                HYDU_ASSERT(!closed, status);
            }
        }
        goto fn_exit;
    } else {
        pmi_storage_len += linelen;
        pmi_storage[pmi_storage_len] = 0;
    }

    int buflen;
    int pmi_version;
  check_cmd:
    status = check_pmi_cmd(&buf, &buflen, &pmi_version, &repeat);
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

    status = handle_pmi_cmd(fd, buf, buflen, pmi_version);
    HYDU_ERR_POP(status, "unable to handle PMI command\n");

    if (repeat)
        /* there are more commands to process. */
        goto check_cmd;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_pmi_response(int fd, int buflen, int pmi_version, int pid)
{
    int count, closed, sent;
    char *buf = NULL;
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(buf, char *, buflen + 1, status);

    status = HYDU_sock_read(fd, buf, buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI response from proxy\n");
    HYDU_ASSERT(!closed, status);

    buf[buflen] = 0;

    struct HYD_pmcd_pmip_pmi_handle pmi_handle_fns[] = {
        {"keyval_cache", fn_keyval_cache},
        {"barrier_out", fn_barrier_out},
        {"\0", NULL}
    };

    char *cmd = PMIU_wire_get_cmd(buf, buflen, pmi_version);

    h = pmi_handle_fns;
    while (h->handler) {
        if (strcmp(cmd, h->cmd) == 0) {
            struct PMIU_cmd pmi;
            /* note: buf is modified during parsing; make sure do not forward */
            status = PMIU_cmd_parse(buf, buflen, pmi_version, &pmi);
            HYDU_ERR_POP(status, "unable to parse PMI command\n");

            if (HYD_pmcd_pmip.user_global.debug) {
                HYDU_dump(stdout, "got pmi from server\n    ");
                HYD_pmcd_pmi_dump(&pmi);
            }

            status = h->handler(fd, &pmi);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand the response %s; forwarding downstream\n", cmd);
    }

    status = HYDU_sock_write(pid, buf, buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to forward PMI response to MPI process\n");

    if (HYD_pmcd_pmip.user_global.auto_cleanup) {
        HYDU_ASSERT(!closed, status);
    } else {
        /* Ignore the error and drop the PMI response */
    }

  fn_exit:
    MPL_free(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_listen_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd = -1;
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
    if (-1 != accept_fd)
        close(accept_fd);
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

static HYD_status singleton_init(void)
{
    HYD_status status = HYD_SUCCESS;
    int sent, recvd, closed;

    HYD_pmcd_pmip.local.proxy_process_count = 1;
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.out, int *, sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.err, int *, sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pid, int *, sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.exit_status, int *, sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pmi_rank, int *, sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pmi_fd, int *, sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.downstream.pmi_fd_active, int *, sizeof(int), status);

    HYD_pmcd_pmip.downstream.out[0] = 0;
    HYD_pmcd_pmip.downstream.err[0] = 0;
    HYD_pmcd_pmip.downstream.pid[0] = HYD_pmcd_pmip.user_global.singleton_pid;
    HYD_pmcd_pmip.downstream.exit_status[0] = PMIP_EXIT_STATUS_UNSET;
    HYD_pmcd_pmip.downstream.pmi_rank[0] = 0;
    HYD_pmcd_pmip.downstream.pmi_fd[0] = HYD_FD_UNSET;
    HYD_pmcd_pmip.downstream.pmi_fd_active[0] = 1;

    int fd;
    status =
        HYDU_sock_connect("localhost", (uint16_t) HYD_pmcd_pmip.user_global.singleton_port, &fd, 0,
                          HYD_CONNECT_DELAY);
    HYDU_ERR_POP(status, "unable to connect to singleton process\n");
    HYD_pmcd_pmip.downstream.pmi_fd[0] = fd;

    char msg[1024];
    strcpy(msg, "cmd=singinit authtype=none\n");
    status = HYDU_sock_write(fd, msg, (int) strlen(msg), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send msg to singleton process\n");
    status = HYDU_sock_read(fd, msg, 1024, &recvd, &closed, HYDU_SOCK_COMM_NONE);
    HYDU_ERR_POP(status, "unable to read msg from singleton process\n");
    MPL_snprintf(msg, 1024, "cmd=singinit_info versionok=yes stdio=no kvsname=%s\n",
                 HYD_pmcd_pmip.local.kvs->kvsname);
    status = HYDU_sock_write(fd, msg, (int) strlen(msg), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send msg to singleton process\n");

    status = HYDT_dmx_register_fd(1, &fd, HYD_POLLIN, NULL, pmi_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Send the PID list upstream */
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PID_LIST;
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.pid,
                             (int) (HYD_pmcd_pmip.local.proxy_process_count * sizeof(int)), &sent,
                             &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PID list upstream\n");
    HYDU_ASSERT(!closed, status);

    /* skip HYDT_dmx_register_fd */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status launch_procs(void)
{
    int i, j, process_id, dummy;
    char *str, *envstr, *list, *pmi_port = NULL;
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
        HYD_pmcd_pmip.downstream.exit_status[i] = PMIP_EXIT_STATUS_UNSET;

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

    if (HYD_pmcd_pmip.user_global.pmi_port) {
        status = HYDU_sock_create_and_listen_portstr(HYD_pmcd_pmip.user_global.iface,
                                                     NULL, NULL, &pmi_port, pmi_listen_cb, NULL);
        HYDU_ERR_POP(status, "unable to create PMI port\n");

        status = HYDU_env_create(&env, "PMI_PORT", pmi_port);
        HYDU_ERR_POP(status, "unable to create env\n");
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
        } else if (!strcmp(exec->env_prop, "all")) {
            /* user explicitly asked us to pass all the environment */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(env->env_name, env->env_value, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        } else if (!strncmp(exec->env_prop, "list", strlen("list"))) {
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
            } else if (HYD_pmcd_pmip.local.hostname) {
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
            MPL_free(str);

            str = HYDU_int_to_str(process_id);
            status = HYDU_append_env_to_list("MPI_LOCALRANKID", str, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
            MPL_free(str);

            if (HYD_pmcd_pmip.user_global.gpus_per_proc == HYD_GPUS_PER_PROC_AUTO) {
                /* nothing to do */
            } else if (HYD_pmcd_pmip.user_global.gpus_per_proc == 0) {
                str = HYDU_int_to_str(-1);

                status = HYDU_append_env_to_list("CUDA_VISIBLE_DEVICES", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                MPL_free(str);
            } else {
                char cuda_str[MAX_GPU_STR_LEN] = { 0 };
                int cuda_str_offset = 0;

                for (int k = 0; k < HYD_pmcd_pmip.user_global.gpus_per_proc; k++) {
                    int p = process_id * HYD_pmcd_pmip.user_global.gpus_per_proc + k;
                    str = HYDU_int_to_str(p);

                    if (k) {
                        MPL_strncpy(cuda_str + cuda_str_offset, ",",
                                    MAX_GPU_STR_LEN - cuda_str_offset);
                        cuda_str_offset++;
                    }
                    MPL_strncpy(cuda_str + cuda_str_offset, str, MAX_GPU_STR_LEN - cuda_str_offset);
                    cuda_str_offset += (int) strlen(str);

                    MPL_free(str);
                }

                status = HYDU_append_env_to_list("CUDA_VISIBLE_DEVICES", cuda_str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }

            if (HYD_pmcd_pmip.user_global.pmi_port) {
                /* If we are using the PMI_PORT format */

                /* PMI_PORT */
                status = HYDU_append_env_to_list("PMI_PORT", pmi_port, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                /* PMI_ID */
                str = HYDU_int_to_str(HYD_pmcd_pmip.downstream.pmi_rank[process_id]);
                status = HYDU_append_env_to_list("PMI_ID", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);
            } else {
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
    hdr.cmd = CMD_PID_LIST;
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.pid,
                             (int) (HYD_pmcd_pmip.local.proxy_process_count * sizeof(int)), &sent,
                             &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PID list upstream\n");
    HYDU_ASSERT(!closed, status);

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

    /* Set default values */
    if (HYD_pmcd_pmip.user_global.topolib == NULL && HYDRA_DEFAULT_TOPOLIB != NULL) {
        /* need to prevent compiler seeing MPL_strdup(NULL) or it will warn */
        const char *topolib = HYDRA_DEFAULT_TOPOLIB;
        HYD_pmcd_pmip.user_global.topolib = MPL_strdup(topolib);
    }

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

    if (hdr.cmd == CMD_PROC_INFO) {
        status = procinfo(fd);
        HYDU_ERR_POP(status, "error parsing process info\n");

        if (HYD_pmcd_pmip.user_global.singleton_port > 0) {
            status = singleton_init();
            HYDU_ERR_POP(status, "singleton_init returned error\n");
        } else {
            status = launch_procs();
            HYDU_ERR_POP(status, "launch_procs returned error\n");
        }
    } else if (hdr.cmd == CMD_PMI_RESPONSE) {
        status = handle_pmi_response(fd, hdr.buflen, hdr.u.pmi.pmi_version, hdr.u.pmi.pid);
        HYDU_ERR_POP(status, "unable to handle PMI response\n");
    } else if (hdr.cmd == CMD_SIGNAL) {
        int signum = hdr.u.data;
        /* FIXME: This code needs to change from sending the signal to
         * a PMI-2 notification message. */
        HYD_pmcd_pmip_send_signal(signum);
    } else if (hdr.cmd == CMD_STDIN) {
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
            } else if (closed) {
                close(HYD_pmcd_pmip.downstream.in);
                HYD_pmcd_pmip.downstream.in = HYD_FD_CLOSED;
            }

            MPL_free(buf);
        } else {
            close(HYD_pmcd_pmip.downstream.in);
            HYD_pmcd_pmip.downstream.in = HYD_FD_CLOSED;
        }
    } else {
        status = HYD_INTERNAL_ERROR;
    }

    HYDU_ERR_POP(status, "error handling proxy command\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
