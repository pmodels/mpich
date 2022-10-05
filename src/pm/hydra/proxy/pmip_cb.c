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

static int pmi_storage_len = 0;
static char pmi_storage[HYD_TMPBUF_SIZE], *sptr = pmi_storage;

HYD_status PMIP_send_hdr_upstream(struct pmip_pg *pg, struct HYD_pmcd_hdr *hdr,
                                  void *buf, int buflen)
{
    HYD_status status = HYD_SUCCESS;
    int upstream_sock_closed, sent;

    hdr->pgid = pg->pgid;
    hdr->proxy_id = pg->proxy_id;
    hdr->buflen = buflen;

    HYDU_ASSERT(hdr->cmd, status);
    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "Sending upstream hdr.cmd = %s\n", HYD_pmcd_cmd_name(hdr->cmd));
    }

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, hdr, sizeof(*hdr), &sent,
                             &upstream_sock_closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "sock write error\n");
    HYDU_ASSERT(!upstream_sock_closed, status);

    if (buflen > 0) {
        status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, buflen, &sent,
                                 &upstream_sock_closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "sock write error\n");
        HYDU_ASSERT(!upstream_sock_closed, status);
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

static HYD_status stdoe_cb(struct pmip_downstream *p, bool is_stdout)
{
    int closed, recvd;
    char buf[HYD_TMPBUF_SIZE];
    struct HYD_pmcd_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int fd = is_stdout ? p->out : p->err;

    status = HYDU_sock_read(fd, buf, HYD_TMPBUF_SIZE, &recvd, &closed, HYDU_SOCK_COMM_NONE);
    HYDU_ERR_POP(status, "sock read error\n");

    if (recvd) {
        hdr.cmd = is_stdout ? CMD_STDOUT : CMD_STDERR;
        hdr.u.io.rank = p->pmi_rank;

        status = PMIP_send_hdr_upstream(PMIP_pg_from_downstream(p), &hdr, buf, recvd);
        HYDU_ERR_POP(status, "error sending hdr upstream\n");
    }

    if (closed) {
        /* The connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        if (is_stdout) {
            p->out = HYD_FD_CLOSED;
        } else {
            p->err = HYD_FD_CLOSED;
        }
        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status stdout_cb(int fd, HYD_event_t events, void *userp)
{
    struct pmip_downstream *p = userp;
    return stdoe_cb(p, true);
}

static HYD_status stderr_cb(int fd, HYD_event_t events, void *userp)
{
    struct pmip_downstream *p = userp;
    return stdoe_cb(p, false);
}

static HYD_status handle_pmi_cmd(struct pmip_downstream *p, char *buf, int buflen, int pmi_version)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    char *cmd = PMIU_wire_get_cmd(buf, buflen, pmi_version);
    int cmd_id = PMIU_msg_cmd_to_id(cmd);

    HYD_status(*handler) (struct pmip_downstream * p, struct PMIU_cmd * pmi) = NULL;
    HYD_status(*init_handler) (int fd, struct PMIU_cmd * pmi) = NULL;
    switch (cmd_id) {
        case PMIU_CMD_INIT:
            handler = fn_init;
            break;
        case PMIU_CMD_FINALIZE:
            handler = fn_finalize;
            break;
        case PMIU_CMD_FULLINIT:
            /* no valid downstream yet */
            init_handler = fn_fullinit;
            break;
        case PMIU_CMD_MAXES:
            handler = fn_get_maxes;
            break;
        case PMIU_CMD_APPNUM:
            handler = fn_get_appnum;
            break;
        case PMIU_CMD_KVSNAME:
            handler = fn_get_my_kvsname;
            break;
        case PMIU_CMD_UNIVERSE:
            handler = fn_get_usize;
            break;
        case PMIU_CMD_GET:
            handler = fn_get;
            break;
        case PMIU_CMD_PUT:
            handler = fn_put;
            break;
        case PMIU_CMD_BARRIER:
            handler = fn_barrier_in;
            break;
        case PMIU_CMD_PUTNODEATTR:
            handler = fn_info_putnodeattr;
            break;
        case PMIU_CMD_GETNODEATTR:
            handler = fn_info_getnodeattr;
            break;
    }

    if (handler || init_handler) {
        struct PMIU_cmd pmi;
        status = PMIU_cmd_parse(buf, buflen, pmi_version, &pmi);
        HYDU_ERR_POP(status, "unable to parse PMI command\n");

        if (HYD_pmcd_pmip.user_global.debug) {
            HYDU_dump(stdout, "got pmi command\n    ");
            HYD_pmcd_pmi_dump(&pmi);
        }

        if (handler) {
            HYDU_ASSERT(p->pg_idx != -1, status);
            status = handler(p, &pmi);
        } else if (init_handler) {
            status = init_handler(p->pmi_fd, &pmi);
        }
        HYDU_ERR_POP(status, "PMI handler returned error\n");
        goto fn_exit;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand this command, forwarding upstream\n");
        HYDU_dump(stdout, "    %s\n", buf);
    }

    /* We don't understand the command; forward it upstream */
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PMI;
    hdr.u.pmi.pmi_version = pmi_version;
    hdr.u.pmi.process_fd = p->pmi_fd;

    HYDU_ASSERT(p->pg_idx != -1, status);
    status = PMIP_send_hdr_upstream(PMIP_pg_from_downstream(p), &hdr, buf, buflen);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");

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

    *pmi_version = 0;
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

    int full_command, buflen, cmd_id;
    int pmi_errno;
    pmi_errno = PMIU_check_full_cmd(sptr, pmi_storage_len, &full_command, &buflen,
                                    pmi_version, &cmd_id);
    assert(!pmi_errno);

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
        *buflen_out = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
}

static HYD_status pmi_cb(int fd, HYD_event_t events, void *userp)
{
    char *buf = NULL;
    int closed, linelen;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct pmip_downstream init_p;
    struct pmip_downstream *p = userp;
    if (p == NULL) {
        /* pmi_port path. We need find the downstream */
        p = PMIP_find_downstream_by_fd(fd);
        if (!p) {
            /* when init via pmi_port, we don't have the matching downstream yet,
             * use init_p. We just need pass on the fd */
            init_p.pg_idx = -1; /* sentinel as not a real downstream */
            init_p.pmi_fd = fd;
            p = &init_p;
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
        if (p->pg_idx != -1 && p->pmi_fd_active) {
            /* Deregister failed socket */
            status = HYDT_dmx_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to deregister fd\n");
            close(fd);

            if (HYD_pmcd_pmip.user_global.auto_cleanup) {
                /* kill all processes */
                /* preset all exit_status except for the closed pid */
                struct pmip_pg *pg = PMIP_pg_from_downstream(p);
                for (int i = 0; i < pg->num_procs; i++) {
                    if (p != &pg->downstreams[i] &&
                        pg->downstreams[i].exit_status == PMIP_EXIT_STATUS_UNSET) {
                        pg->downstreams[i].exit_status = 0;
                    }
                }
                PMIP_bcast_signal(SIGKILL);
            } else {
                /* If the user doesn't want to automatically cleanup,
                 * signal the remaining processes, and send this
                 * information upstream */

                /* FIXME: This code needs to change from sending the
                 * SIGUSR1 signal to a PMI-2 notification message. */
                PMIP_bcast_signal(SIGUSR1);

                struct HYD_pmcd_hdr hdr;
                HYD_pmcd_init_header(&hdr);

                hdr.cmd = CMD_PROCESS_TERMINATED;
                /* global rank for the terminated process */
                hdr.u.data = p->pmi_rank;

                HYDU_ASSERT(p, status);
                status = PMIP_send_hdr_upstream(PMIP_pg_from_downstream(p), &hdr, NULL, 0);
                HYDU_ERR_POP(status, "unable to send hdr upstream\n");
            }
        }
        goto fn_exit;
    } else {
        pmi_storage_len += linelen;
        pmi_storage[pmi_storage_len] = 0;
    }

    int repeat;
    do {
        int buflen = 0;
        int pmi_version;
        status = check_pmi_cmd(&buf, &buflen, &pmi_version, &repeat);
        HYDU_ERR_POP(status, "error checking the PMI command\n");

        if (buf == NULL)
            /* read more to get a full command. */
            goto read_cmd;

        /* We were able to read the PMI command correctly. If we were able
         * to identify what PMI FD this is, activate it. If we were not
         * able to identify the PMI FD, we will activate it when we get
         * the PMI initialization command. */
        if (p->pg_idx != -1 && !p->pmi_fd_active) {
            p->pmi_fd_active = 1;
        }

        status = handle_pmi_cmd(p, buf, buflen, pmi_version);
        HYDU_ERR_POP(status, "unable to handle PMI command\n");
    } while (repeat);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_pmi_response(struct pmip_pg *pg, int buflen, int pmi_version,
                                      int process_fd)
{
    int count, closed, sent;
    char *buf = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(buf, char *, buflen + 1, status);

    status = HYDU_sock_read(HYD_pmcd_pmip.upstream.control, buf, buflen,
                            &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI response from proxy\n");
    HYDU_ASSERT(!closed, status);

    buf[buflen] = 0;

    char *cmd = PMIU_wire_get_cmd(buf, buflen, pmi_version);
    int cmd_id = PMIU_msg_cmd_to_id(cmd);

    HYD_status(*handler) (struct pmip_pg * pg, struct PMIU_cmd * pmi) = NULL;
    if (cmd_id == PMIU_CMD_KVSCACHE) {
        handler = fn_keyval_cache;
    } else if (cmd_id == PMIU_CMD_BARRIEROUT) {
        handler = fn_barrier_out;
    }

    if (handler) {
        struct PMIU_cmd pmi;
        /* note: buf is modified during parsing; make sure do not forward */
        status = PMIU_cmd_parse(buf, buflen, pmi_version, &pmi);
        HYDU_ERR_POP(status, "unable to parse PMI command\n");

        status = handler(pg, &pmi);
        HYDU_ERR_POP(status, "PMI handler returned error\n");

        PMIU_cmd_free_buf(&pmi);
        goto fn_exit;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "we don't understand the response %s; forwarding downstream\n", cmd);
    }

    status = HYDU_sock_write(process_fd, buf, buflen, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
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

static HYD_status handle_launch_procs(struct pmip_pg *pg);

HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    int cmd_len, closed;
    struct HYD_pmcd_hdr hdr;
    char *buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();
    HYDU_ASSERT(fd == HYD_pmcd_pmip.upstream.control, status);

    /* We got a command from upstream */
    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &cmd_len, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading command from launcher\n");
    HYDU_ASSERT(!closed, status);

    struct pmip_pg *pg;
    pg = PMIP_find_pg(hdr.pgid, hdr.proxy_id);

    if (hdr.cmd == CMD_PROC_INFO) {
        HYDU_ASSERT(pg == NULL, status);
        pg = PMIP_new_pg(hdr.pgid, hdr.proxy_id);
        HYDU_ASSERT(pg, status);

        status = handle_launch_procs(pg);
        HYDU_ERR_POP(status, "launch_procs returned error\n");
    } else if (hdr.cmd == CMD_PMI_RESPONSE) {
        status = handle_pmi_response(pg, hdr.buflen, hdr.u.pmi.pmi_version, hdr.u.pmi.process_fd);
        HYDU_ERR_POP(status, "unable to handle PMI response\n");
    } else if (hdr.cmd == CMD_SIGNAL) {
        int signum = hdr.u.data;
        /* FIXME: This code needs to change from sending the signal to
         * a PMI-2 notification message. */
        PMIP_bcast_signal(signum);
    } else if (hdr.cmd == CMD_STDIN) {
        HYDU_ASSERT(pg, status);
        /* stdin is connected to the first local process */
        struct pmip_downstream *p = &pg->downstreams[0];
        HYDU_ASSERT(p, status);

        if (hdr.buflen) {
            HYDU_MALLOC_OR_JUMP(buf, char *, hdr.buflen, status);
            HYDU_ERR_POP(status, "unable to allocate memory\n");

            int count;
            status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "unable to read from control socket\n");
            HYDU_ASSERT(!closed, status);

            if (p->in == HYD_FD_CLOSED) {
                MPL_free(buf);
                goto fn_exit;
            }

            status = HYDU_sock_write(p->in, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_NONE);
            HYDU_ERR_POP(status, "unable to write to downstream stdin\n");

            HYDU_ERR_CHKANDJUMP(status, count != hdr.buflen, HYD_INTERNAL_ERROR,
                                "process reading stdin too slowly; can't keep up\n");

            HYDU_ASSERT(count == hdr.buflen, status);

            if (HYD_pmcd_pmip.user_global.auto_cleanup) {
                HYDU_ASSERT(!closed, status);
            } else if (closed) {
                close(p->in);
                p->in = HYD_FD_CLOSED;
            }

            MPL_free(buf);
        } else {
            close(p->in);
            p->in = HYD_FD_CLOSED;
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

/* handle_launch_procs - read proc_info and launch procs */

static HYD_status parse_exec_params(struct pmip_pg *pg, char **t_argv);
static HYD_status procinfo(struct pmip_pg *pg);
static HYD_status singleton_init(struct pmip_pg *pg, int singleton_pid, int singleton_port);
static HYD_status launch_procs(struct pmip_pg *pg);
static void init_pg_params(struct pmip_pg *pg);
static HYD_status verify_pg_params(struct pmip_pg *pg);
static int local_to_global_id(struct pmip_pg *pg, int local_id);

static HYD_status handle_launch_procs(struct pmip_pg *pg)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    status = procinfo(pg);
    HYDU_ERR_POP(status, "error parsing process info\n");

    /* FIXME: split topo initialization from applying bindings.
     *        The topolib should be passed as proxy commandline args
     *        and initialized in main.
     */
    status = HYDT_topo_init(HYD_pmcd_pmip.user_global.topolib,
                            HYD_pmcd_pmip.user_global.binding,
                            HYD_pmcd_pmip.user_global.mapping, HYD_pmcd_pmip.user_global.membind);
    HYDU_ERR_POP(status, "unable to initialize process topology\n");

    if (pg->is_singleton) {
        status = singleton_init(pg, HYD_pmcd_pmip.singleton_pid, HYD_pmcd_pmip.singleton_port);
        HYDU_ERR_POP(status, "singleton_init returned error\n");
    } else {
        status = launch_procs(pg);
        HYDU_ERR_POP(status, "launch_procs returned error\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

static HYD_status parse_exec_params(struct pmip_pg *pg, char **t_argv)
{
    char **argv = t_argv;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_set_cur_pg(pg);
    init_pg_params(pg);
    do {
        /* Get the executable arguments  */
        status = HYDU_parse_array(&argv, HYD_pmcd_pmip_match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        /* No more arguments left */
        if (!(*argv))
            break;
    } while (1);

    /* verify the arguments we got */
    status = verify_pg_params(pg);
    HYDU_ERR_POP(status, "missing parameters\n");

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

static HYD_status procinfo(struct pmip_pg *pg)
{
    char **arglist;
    int num_strings, str_len, recvd, i, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int fd = HYD_pmcd_pmip.upstream.control;

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
    status = parse_exec_params(pg, arglist);
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

static HYD_status singleton_init(struct pmip_pg *pg, int singleton_pid, int singleton_port)
{
    HYD_status status = HYD_SUCCESS;
    int sent, recvd, closed;

    status = PMIP_pg_alloc_downstreams(pg, 1);
    HYDU_ERR_POP(status, "unable to allocate singleton downstreams\n");

    struct pmip_downstream *p;
    p = &pg->downstreams[0];

    int fd;
    status = HYDU_sock_connect("localhost", singleton_port, &fd, 0, HYD_CONNECT_DELAY);
    HYDU_ERR_POP(status, "unable to connect to singleton process\n");

    p->pid = singleton_pid;
    p->exit_status = PMIP_EXIT_STATUS_UNSET;
    p->pmi_rank = 0;
    p->pmi_fd = fd;
    p->pmi_fd_active = 1;

    char msg[1024];
    strcpy(msg, "cmd=singinit authtype=none\n");
    status = HYDU_sock_write(fd, msg, strlen(msg), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send msg to singleton process\n");
    status = HYDU_sock_read(fd, msg, 1024, &recvd, &closed, HYDU_SOCK_COMM_NONE);
    HYDU_ERR_POP(status, "unable to read msg from singleton process\n");
    snprintf(msg, 1024, "cmd=singinit_info versionok=yes stdio=no kvsname=%s\n", pg->kvsname);
    status = HYDU_sock_write(fd, msg, strlen(msg), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send msg to singleton process\n");

    status = HYDT_dmx_register_fd(1, &fd, HYD_POLLIN, NULL, pmi_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Send the PID list upstream */
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PID_LIST;

    status = PMIP_send_hdr_upstream(pg, &hdr, &singleton_pid, sizeof(int));
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");

    /* skip HYDT_dmx_register_fd */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status launch_procs(struct pmip_pg *pg)
{
    int j, process_id, dummy;
    char *str, *envstr, *list, *pmi_port = NULL;
    struct HYD_string_stash stash;
    struct HYD_env *env, *force_env = NULL;
    struct HYD_exec *exec;
    struct HYD_pmcd_hdr hdr;
    int pmi_fds[2] = { HYD_FD_UNSET, HYD_FD_UNSET };
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int num_procs = 0;
    for (exec = pg->exec_list; exec; exec = exec->next) {
        num_procs += exec->proc_count;
    }

    status = PMIP_pg_alloc_downstreams(pg, num_procs);
    HYDU_ERR_POP(status, "unable to allocate singleton downstreams\n");

    /* Initialize the PMI_FD and PMI FD active state, and exit status */
    for (int i = 0; i < num_procs; i++) {
        struct pmip_downstream *p = &pg->downstreams[i];
        /* The exit status is populated when the processes terminate */
        p->exit_status = PMIP_EXIT_STATUS_UNSET;

        /* If we use PMI_FD, the pmi_fd and pmi_fd_active will
         * be filled out in this function. But if we are using
         * PMI_PORT, we will fill them out later when the processes
         * send the PMI initialization message. Note that non-MPI
         * processes are never "PMI active" when we use the PMI
         * PORT. */
        p->pmi_fd = HYD_FD_UNSET;
        p->pmi_fd_active = 0;
        p->pmi_rank = local_to_global_id(pg, i);
    }

    if (HYD_pmcd_pmip.user_global.pmi_port) {
        int listen_fd;
        status = HYDU_sock_create_and_listen_portstr(HYD_pmcd_pmip.user_global.iface,
                                                     NULL, NULL, &pmi_port, &listen_fd,
                                                     pmi_listen_cb, NULL);
        HYDU_ERR_POP(status, "unable to create PMI port\n");

        /* FIXME: should we close(listen_fd) at some point? */

        status = HYDU_env_create(&env, "PMI_PORT", pmi_port);
        HYDU_ERR_POP(status, "unable to create env\n");
    }

    /* Spawn the processes */
    process_id = 0;
    for (exec = pg->exec_list; exec; exec = exec->next) {

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

        bool allocate_subdev = false;
        int n_local_gpus = 0;
        if (HYD_pmcd_pmip.user_global.gpu_subdevs_per_proc != HYD_GPUS_PER_PROC_AUTO) {
            allocate_subdev = true;
            n_local_gpus = HYD_pmcd_pmip.user_global.gpu_subdevs_per_proc;
        } else {
            allocate_subdev = false;
            n_local_gpus = HYD_pmcd_pmip.user_global.gpus_per_proc;
        }
        for (int i = 0; i < exec->proc_count; i++) {
            struct pmip_downstream *p = &pg->downstreams[process_id];
            p->pmi_appnum = exec->appnum;

            /* FIXME: these envvars should be set by MPICH instead. See #2360 */
            str = HYDU_int_to_str(pg->num_procs);
            status = HYDU_append_env_to_list("MPI_LOCALNRANKS", str, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
            MPL_free(str);

            str = HYDU_int_to_str(process_id);
            status = HYDU_append_env_to_list("MPI_LOCALRANKID", str, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
            MPL_free(str);

            if (HYD_pmcd_pmip.user_global.gpus_per_proc == HYD_GPUS_PER_PROC_AUTO
                && HYD_pmcd_pmip.user_global.gpu_subdevs_per_proc == HYD_GPUS_PER_PROC_AUTO) {
                /* nothing to do */
            } else if (HYD_pmcd_pmip.user_global.gpus_per_proc == 0
                       || HYD_pmcd_pmip.user_global.gpu_subdevs_per_proc == 0) {
                MPL_gpu_dev_affinity_to_env(0, NULL, &str);

                status = HYDU_append_env_to_list(MPL_GPU_DEV_AFFINITY_ENV, str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

            } else {
                int dev_count = 0;
                int n_dev_ids = 0;
                int max_dev_id = 0;
                int max_subdev_id = 0;
                char **all_dev_ids = NULL;
                char **child_dev_ids = NULL;
                char *affinity_env_str = NULL;
                int n_gpu_assgined = 0;

                MPL_gpu_get_dev_count(&dev_count, &max_dev_id, &max_subdev_id);
                MPL_gpu_get_dev_list(&n_dev_ids, &all_dev_ids, allocate_subdev);
                child_dev_ids = (char **) MPL_malloc(n_local_gpus * sizeof(char *), MPL_MEM_OTHER);
                HYDU_ASSERT(child_dev_ids, status);

                for (int k = 0; k < n_local_gpus; k++) {
                    int idx = process_id * n_local_gpus + k;

                    if (idx >= n_dev_ids) {
                        break;
                    }
                    int id_str_len = strlen(all_dev_ids[idx]);
                    child_dev_ids[k] = (char *) MPL_malloc((id_str_len + 1) * sizeof(char),
                                                           MPL_MEM_OTHER);
                    MPL_strncpy(child_dev_ids[k], all_dev_ids[idx], id_str_len + 1);
                    n_gpu_assgined++;
                }

                MPL_gpu_dev_affinity_to_env(n_gpu_assgined, child_dev_ids, &affinity_env_str);

                status = HYDU_append_env_to_list(MPL_GPU_DEV_AFFINITY_ENV, affinity_env_str,
                                                 &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }

            if (HYD_pmcd_pmip.user_global.pmi_port) {
                /* If we are using the PMI_PORT format */

                /* PMI_PORT */
                status = HYDU_append_env_to_list("PMI_PORT", pmi_port, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                /* PMI_ID */
                str = HYDU_int_to_str(p->pmi_rank);
                status = HYDU_append_env_to_list("PMI_ID", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);
            } else {
                /* PMI_RANK */
                str = HYDU_int_to_str(p->pmi_rank);
                status = HYDU_append_env_to_list("PMI_RANK", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);

                if (socketpair(AF_UNIX, SOCK_STREAM, 0, pmi_fds) < 0)
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

                status = HYDT_dmx_register_fd(1, &pmi_fds[0], HYD_POLLIN, p, pmi_cb);
                HYDU_ERR_POP(status, "unable to register fd\n");

                status = HYDU_sock_cloexec(pmi_fds[0]);
                HYDU_ERR_POP(status, "unable to set socket to close on exec\n");

                p->pmi_fd = pmi_fds[0];
                str = HYDU_int_to_str(pmi_fds[1]);

                status = HYDU_append_env_to_list("PMI_FD", str, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
                MPL_free(str);

                /* PMI_SIZE */
                str = HYDU_int_to_str(pg->global_process_count);
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
                                         p->pmi_rank ? &dummy : &p->in,
                                         &p->out, &p->err, &p->pid, process_id);
            HYDU_ERR_POP(status, "create process returned error\n");

            if (p->in != HYD_FD_UNSET) {
                status = HYDU_sock_set_nonblock(p->in);
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

    int *pid_list = PMIP_pg_get_pid_list(pg);
    status = PMIP_send_hdr_upstream(pg, &hdr, pid_list, num_procs * sizeof(int));
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");
    MPL_free(pid_list);

    /* Everything is spawned, register the required FDs  */
    for (int i = 0; i < num_procs; i++) {
        struct pmip_downstream *p = &pg->downstreams[i];

        status = HYDT_dmx_register_fd(1, &p->out, HYD_POLLIN, p, stdout_cb);
        HYDU_ERR_POP(status, "unable to register fd\n");

        status = HYDT_dmx_register_fd(1, &p->err, HYD_POLLIN, p, stderr_cb);
        HYDU_ERR_POP(status, "unable to register fd\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* global_core_map and pmi_id_map */
static void init_pg_params(struct pmip_pg *pg)
{
    pg->global_core_map.local_filler = -1;
    pg->global_core_map.local_count = -1;
    pg->global_core_map.global_count = -1;
    pg->pmi_id_map.filler_start = -1;
    pg->pmi_id_map.non_filler_start = -1;

    pg->global_process_count = -1;
}

static HYD_status verify_pg_params(struct pmip_pg *pg)
{
    HYD_status status = HYD_SUCCESS;
    if (pg->global_core_map.local_filler == -1 ||
        pg->global_core_map.local_count == -1 || pg->global_core_map.global_count == -1) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "cannot find global core map (%d,%d,%d)\n",
                            pg->global_core_map.local_filler,
                            pg->global_core_map.local_count, pg->global_core_map.global_count);
    }

    if (pg->pmi_id_map.filler_start == -1 || pg->pmi_id_map.non_filler_start == -1) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "cannot find pmi id map (%d,%d)\n",
                            pg->pmi_id_map.filler_start, pg->pmi_id_map.non_filler_start);
    }

    if (pg->global_process_count == -1) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cannot find global_process_count\n");
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

static int local_to_global_id(struct pmip_pg *pg, int local_id)
{
    int rem1, rem2, layer, ret;

    if (local_id < pg->global_core_map.local_filler)
        ret = pg->pmi_id_map.filler_start + local_id;
    else {
        rem1 = local_id - pg->global_core_map.local_filler;
        layer = rem1 / pg->global_core_map.local_count;
        rem2 = rem1 - (layer * pg->global_core_map.local_count);

        ret = pg->pmi_id_map.non_filler_start + (layer * pg->global_core_map.global_count) + rem2;
    }

    return ret;
}
