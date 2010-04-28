/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmip.h"
#include "pmip_pmi.h"
#include "ckpoint.h"
#include "demux.h"
#include "bind.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;
struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_handle = { 0 };

static int storage_len = 0;
static char storage[HYD_TMPBUF_SIZE], *sptr = storage, r[HYD_TMPBUF_SIZE];
static int using_pmi_port = 0;

static HYD_status stdio_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, i, sent, recvd, stdfd;
    char buf[HYD_TMPBUF_SIZE];
    struct HYD_pmcd_stdio_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    stdfd = (int) (size_t) userp;

    if (stdfd == STDIN_FILENO) {
        status = HYDU_sock_forward_stdio(fd, HYD_pmcd_pmip.downstream.in, &closed);
        HYDU_ERR_POP(status, "stdin forwarding error\n");

        if (closed) {
            status = HYDT_dmx_deregister_fd(fd);
            HYDU_ERR_POP(status, "unable to deregister fd\n");

            close(fd);

            close(HYD_pmcd_pmip.downstream.in);
            HYD_pmcd_pmip.downstream.in = HYD_FD_CLOSED;
        }

        goto fn_exit;
    }

    status = HYDU_sock_read(fd, buf, HYD_TMPBUF_SIZE, &recvd, &closed, 0);
    HYDU_ERR_POP(status, "sock read error\n");

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

    if (recvd) {
        if (stdfd == STDOUT_FILENO) {
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.out[i] == fd)
                    break;
        }
        else {
            for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
                if (HYD_pmcd_pmip.downstream.err[i] == fd)
                    break;
        }

        HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

        if (HYD_pmcd_pmip.user_global.prepend_rank) {
            hdr.rank = HYD_pmcd_pmip.downstream.pmi_rank[i];
            hdr.buflen = recvd;

            status = HYDU_sock_write(stdfd, &hdr, sizeof(hdr), &sent, &closed);
            HYDU_ERR_POP(status, "sock write error\n");
            HYDU_ASSERT(!closed, status);
        }

        status = HYDU_sock_write(stdfd, buf, recvd, &sent, &closed);
        HYDU_ERR_POP(status, "sock write error\n");
        HYDU_ASSERT(!closed, status);
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
    if (storage_len < 6)
        goto fn_exit;

    /* FIXME: This should really be a "FIXME" for the client, since
     * there's not much we can do on the server side.
     *
     * We initialize to whatever PMI version we detect while reading
     * the PMI command, instead of relying on what the init command
     * gave us. This part of the code should not know anything about
     * PMI-1 vs. PMI-2. But the simple PMI client-side code in MPICH2
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
            for (bufptr = sptr; bufptr < sptr + storage_len; bufptr++) {
                if (*bufptr == '\n') {
                    full_command = 1;
                    break;
                }
            }
        }
        else {  /* multi commands */
            for (bufptr = sptr; bufptr < sptr + storage_len - strlen("endcmd\n") + 1; bufptr++) {
                if (bufptr[0] == 'e' && bufptr[1] == 'n' && bufptr[2] == 'd' &&
                    bufptr[3] == 'c' && bufptr[4] == 'm' && bufptr[5] == 'd' &&
                    bufptr[6] == '\n') {
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

        if (storage_len >= cmdlen + 6) {
            full_command = 1;
            bufptr = sptr + 6 + cmdlen - 1;
        }
    }

    if (full_command) {
        /* We have a full command */
        buflen = bufptr - sptr + 1;
        HYDU_MALLOC(*buf, char *, buflen, status);
        memcpy(*buf, sptr, buflen);
        sptr += buflen;
        storage_len -= buflen;
        (*buf)[buflen - 1] = '\0';

        if (storage_len == 0)
            sptr = storage;
        else
            *repeat = 1;
    }
    else {
        /* We don't have a full command. Copy the rest of the data to
         * the front of the storage buffer. */

        /* FIXME: This dual memcpy is crazy and needs to be
         * fixed. Single memcpy should be possible, but we need to be
         * a bit careful not to corrupt the buffer. */
        if (sptr != storage) {
            memcpy(r, sptr, storage_len);
            memcpy(storage, r, storage_len);
            storage_len = 0;
            sptr = storage;
        }
        *buf = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status read_pmi_cmd(int fd, int *closed)
{
    int linelen;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *closed = 0;

    /* PMI-1 does not tell us how much to read. We read how much ever
     * we can, parse out full PMI commands from it, and process
     * them. When we don't have a full PMI command, we store the
     * rest. */
    status = HYDU_sock_read(fd, storage + storage_len, HYD_TMPBUF_SIZE - storage_len,
                            &linelen, closed, 0);
    HYDU_ERR_POP(status, "unable to read PMI command\n");

    /* Unexpected termination of connection */
    if (*closed) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(fd);

        goto fn_exit;
    }

    storage_len += linelen;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_cb(int fd, HYD_event_t events, void *userp)
{
    char *buf = NULL, *pmi_cmd, *args[HYD_NUM_TMP_STRINGS];
    int closed, repeat, sent, i;
    struct HYD_pmcd_pmi_hdr hdr;
    enum HYD_pmcd_pmi_cmd cmd;
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = read_pmi_cmd(fd, &closed);
    HYDU_ERR_POP(status, "unable to read PMI command\n");

    /* If we used the PMI_FD format, try to find the PMI FD */
    if (!using_pmi_port) {
        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
            if (HYD_pmcd_pmip.downstream.pmi_fd[i] == fd)
                break;
        HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);
    }

    if (closed) {
        /* This is a hack to improve user-friendliness. If a PMI
         * application terminates, we clean up the remaining
         * processes. For a correct PMI application, we should never
         * get closed socket event as we deregister this socket as
         * soon as we get the finalize message. For non-PMI
         * applications, this is harder to identify, so we just let
         * the user cleanup the processes on a failure. */
        if (using_pmi_port || HYD_pmcd_pmip.downstream.pmi_fd_active[i])
            HYD_pmcd_pmip_killjob();
        goto fn_exit;
    }

    /* This is a PMI application */
    if (!using_pmi_port)
        HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 1;

    do {
        status = check_pmi_cmd(&buf, &hdr.pmi_version, &repeat);
        HYDU_ERR_POP(status, "error checking the PMI command\n");

        if (buf == NULL)
            break;

        if (hdr.pmi_version == 1)
            HYD_pmcd_pmip_pmi_handle = HYD_pmcd_pmip_pmi_v1;
        else
            HYD_pmcd_pmip_pmi_handle = HYD_pmcd_pmip_pmi_v2;

        status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
        HYDU_ERR_POP(status, "unable to parse PMI command\n");

        if (HYD_pmcd_pmip.user_global.debug) {
            HYD_pmcd_pmi_proxy_dump(status, STDOUT_FILENO,
                                    "got pmi command (from %d): %s\n", fd, pmi_cmd);
            HYDU_print_strlist(args);
        }

        h = HYD_pmcd_pmip_pmi_handle;
        while (h->handler) {
            if (!strcmp(pmi_cmd, h->cmd)) {
                status = h->handler(fd, args);
                HYDU_ERR_POP(status, "PMI handler returned error\n");
                goto fn_exit;
            }
            h++;
        }

        if (HYD_pmcd_pmip.user_global.debug) {
            HYD_pmcd_pmi_proxy_dump(status, STDOUT_FILENO,
                                    "we don't understand this command %s; forwarding upstream\n",
                                    pmi_cmd);
        }

        /* We don't understand the command; forward it upstream */
        cmd = PMI_CMD;
        status =
            HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &cmd, sizeof(cmd), &sent, &closed);
        HYDU_ERR_POP(status, "unable to send PMI_CMD command\n");
        HYDU_ASSERT(!closed, status);

        hdr.pid = fd;
        hdr.buflen = strlen(buf);
        status =
            HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed);
        HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
        HYDU_ASSERT(!closed, status);

        status =
            HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed);
        HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
        HYDU_ASSERT(!closed, status);

    } while (repeat);

  fn_exit:
    if (buf)
        HYDU_FREE(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_pmi_response(int fd)
{
    struct HYD_pmcd_pmi_hdr hdr;
    int count, closed, sent;
    char *buf = NULL, *pmi_cmd, *args[HYD_NUM_TMP_STRINGS];
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI header from proxy\n");
    HYDU_ASSERT(!closed, status);

    HYDU_MALLOC(buf, char *, hdr.buflen + 1, status);

    status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI response from proxy\n");
    HYDU_ASSERT(!closed, status);

    buf[hdr.buflen] = 0;

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

    h = HYD_pmcd_pmip_pmi_handle;
    while (h->handler) {
        if (!strcmp(pmi_cmd, h->cmd)) {
            status = h->handler(fd, args);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYD_pmcd_pmi_proxy_dump(status, STDOUT_FILENO,
                                "we don't understand the response %s; forwarding downstream\n",
                                pmi_cmd);
    }

    status = HYDU_sock_write(hdr.pid, buf, hdr.buflen, &sent, &closed);
    HYDU_ERR_POP(status, "unable to forward PMI response to MPI process\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
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

static HYD_status launch_procs(void)
{
    int i, j, arg, stdin_fd, process_id, os_index, pmi_rank;
    char *str, *envstr, *list, *pmi_port;
    char *client_args[HYD_NUM_TMP_STRINGS];
    struct HYD_env *env, *opt_env = NULL, *force_env = NULL;
    struct HYD_exec *exec;
    enum HYD_pmcd_pmi_cmd cmd;
    int *pmi_ranks;
    int sent, closed, pmi_fds[2] = { HYD_FD_UNSET, HYD_FD_UNSET };
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_pmip.local.proxy_process_count = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next)
        HYD_pmcd_pmip.local.proxy_process_count += exec->proc_count;

    HYDU_MALLOC(pmi_ranks, int *, HYD_pmcd_pmip.local.proxy_process_count * sizeof(int),
                status);
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        pmi_ranks[i] =
            HYDU_local_to_global_id(i, HYD_pmcd_pmip.start_pid,
                                    HYD_pmcd_pmip.local.proxy_core_count,
                                    HYD_pmcd_pmip.system_global.global_core_count);
    }

    HYDU_MALLOC(HYD_pmcd_pmip.downstream.out, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.err, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.pid, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.exit_status, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.pmi_rank, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.pmi_fd, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.pmi_fd_active, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);

    /* Initialize the PMI_FD and PMI FD active state */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        HYD_pmcd_pmip.downstream.pmi_fd[i] = HYD_FD_UNSET;
        HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 0;
    }

    /* Initialize the exit status */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
        HYD_pmcd_pmip.downstream.exit_status[i] = -1;

    status = HYDT_bind_init(HYD_pmcd_pmip.local.local_binding ?
                            HYD_pmcd_pmip.local.local_binding :
                            HYD_pmcd_pmip.user_global.binding,
                            HYD_pmcd_pmip.user_global.bindlib);
    HYDU_ERR_POP(status, "unable to initialize process binding\n");

    status = HYDT_ckpoint_init(HYD_pmcd_pmip.user_global.ckpointlib,
                               HYD_pmcd_pmip.user_global.ckpoint_prefix);
    HYDU_ERR_POP(status, "unable to initialize checkpointing\n");

    if (HYD_pmcd_pmip.system_global.pmi_port || HYD_pmcd_pmip.user_global.ckpoint_prefix) {
        using_pmi_port = 1;
        if (HYD_pmcd_pmip.system_global.pmi_port)
            pmi_port = HYD_pmcd_pmip.system_global.pmi_port;
        else {
            status = HYDU_sock_create_and_listen_portstr(HYD_pmcd_pmip.user_global.iface,
                                                         NULL, &pmi_port, pmi_listen_cb, NULL);
            HYDU_ERR_POP(status, "unable to create PMI port\n");
        }
    }

    if (HYD_pmcd_pmip.exec_list->exec[0] == NULL) {      /* Checkpoint restart cast */
        status = HYDU_env_create(&env, "PMI_PORT", pmi_port);
        HYDU_ERR_POP(status, "unable to create env\n");

        /* Restart the proxy.  Specify stdin fd only if pmi_rank 0 is in this proxy. */
        status = HYDT_ckpoint_restart(env, HYD_pmcd_pmip.local.proxy_process_count,
                                      pmi_ranks,
                                      pmi_ranks[0] ? NULL :
                                      HYD_pmcd_pmip.system_global.enable_stdin ?
                                      &HYD_pmcd_pmip.downstream.in : NULL,
                                      HYD_pmcd_pmip.downstream.out,
                                      HYD_pmcd_pmip.downstream.err);
        HYDU_ERR_POP(status, "checkpoint restart failure\n");
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
            exec->env_prop = HYDU_strdup(HYD_pmcd_pmip.user_global.global_env.prop);

        if (!exec->env_prop) {
            /* user didn't specify anything; add inherited env to optional env */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &opt_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }

            status = HYDU_env_purge_existing(&opt_env);
            HYDU_ERR_POP(status, "error purging optional environment\n");

            force_env = opt_env;
            opt_env = NULL;
        }
        else if (!strcmp(exec->env_prop, "all")) {
            /* user explicitly asked us to pass all the environment */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (!strncmp(exec->env_prop, "list", strlen("list"))) {
            if (exec->env_prop)
                list = HYDU_strdup(exec->env_prop + strlen("list:"));
            else
                list = HYDU_strdup(HYD_pmcd_pmip.user_global.global_env.prop +
                                   strlen("list:"));

            envstr = strtok(list, ",");
            while (envstr) {
                env = HYDU_env_lookup(envstr, HYD_pmcd_pmip.user_global.global_env.inherited);
                if (env) {
                    status = HYDU_append_env_to_list(*env, &force_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                }
                envstr = strtok(NULL, ",");
            }
        }

        /* global user env */
        for (env = HYD_pmcd_pmip.user_global.global_env.user; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* local user env */
        for (env = exec->user_env; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* system env */
        for (env = HYD_pmcd_pmip.user_global.global_env.system; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* Set the interface hostname based on what the user provided */
        if (HYD_pmcd_pmip.local.interface_env_name) {
            if (HYD_pmcd_pmip.user_global.iface) {
                /* The user asked us to use a specific interface; let's find it */
                status = HYDU_env_create(&env, HYD_pmcd_pmip.local.interface_env_name,
                                         HYD_pmcd_pmip.user_global.iface);
                HYDU_ERR_POP(status, "unable to create env\n");

                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
            else if (HYD_pmcd_pmip.local.hostname) {
                /* The second choice is the hostname the user gave */
                status = HYDU_env_create(&env, HYD_pmcd_pmip.local.interface_env_name,
                                         HYD_pmcd_pmip.local.hostname);
                HYDU_ERR_POP(status, "unable to create env\n");

                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }

        if (exec->wdir && chdir(exec->wdir) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "unable to change wdir to %s (%s)\n", exec->wdir,
                                HYDU_strerror(errno));

        for (i = 0; i < exec->proc_count; i++) {

            if (HYD_pmcd_pmip.system_global.pmi_rank == -1)
                pmi_rank = HYDU_local_to_global_id(process_id,
                                                   HYD_pmcd_pmip.start_pid,
                                                   HYD_pmcd_pmip.local.proxy_core_count,
                                                   HYD_pmcd_pmip.
                                                   system_global.global_core_count);
            else
                pmi_rank = HYD_pmcd_pmip.system_global.pmi_rank;

            HYD_pmcd_pmip.downstream.pmi_rank[process_id] = pmi_rank;

            if (HYD_pmcd_pmip.system_global.pmi_port || HYD_pmcd_pmip.user_global.ckpoint_prefix) {
                /* If a global PMI_PORT is provided, or this is a
                 * checkpointing case, use PMI_PORT format */

                /* PMI_PORT */
                status = HYDU_env_create(&env, "PMI_PORT", pmi_port);
                HYDU_ERR_POP(status, "unable to create env\n");
                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                /* PMI_ID */
                str = HYDU_int_to_str(pmi_rank);
                status = HYDU_env_create(&env, "PMI_ID", str);
                HYDU_ERR_POP(status, "unable to create env\n");
                HYDU_FREE(str);
                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
            else {
                /* PMI_RANK */
                str = HYDU_int_to_str(pmi_rank);
                status = HYDU_env_create(&env, "PMI_RANK", str);
                HYDU_ERR_POP(status, "unable to create env\n");
                HYDU_FREE(str);
                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                /* PMI_FD */
                if (HYD_pmcd_pmip.system_global.pmi_fd) {
                    /* If a global PMI port is provided, use it */
                    status = HYDU_env_create(&env, "PMI_FD", HYD_pmcd_pmip.system_global.pmi_fd);
                    HYDU_ERR_POP(status, "unable to create env\n");
                }
                else {
                    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pmi_fds) < 0)
                        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "pipe error\n");

                    status = HYDT_dmx_register_fd(1, &pmi_fds[0], HYD_POLLIN, NULL, pmi_cb);
                    HYDU_ERR_POP(status, "unable to register fd\n");

                    str = HYDU_int_to_str(pmi_fds[1]);
                    status = HYDU_env_create(&env, "PMI_FD", str);
                    HYDU_ERR_POP(status, "unable to create env\n");
                    HYDU_FREE(str);

                    HYD_pmcd_pmip.downstream.pmi_fd[process_id] = pmi_fds[0];
                }

                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");

                /* PMI_SIZE */
                str = HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_process_count);
                status = HYDU_env_create(&env, "PMI_SIZE", str);
                HYDU_ERR_POP(status, "unable to create env\n");
                HYDU_FREE(str);
                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }

            for (j = 0, arg = 0; exec->exec[j]; j++)
                client_args[arg++] = HYDU_strdup(exec->exec[j]);
            client_args[arg++] = NULL;

            os_index = HYDT_bind_get_os_index(process_id);
            if (pmi_rank == 0) {
                status = HYDU_create_process(client_args, force_env,
                                             HYD_pmcd_pmip.system_global.enable_stdin ?
                                             &HYD_pmcd_pmip.downstream.in : NULL,
                                             &HYD_pmcd_pmip.downstream.out[process_id],
                                             &HYD_pmcd_pmip.downstream.err[process_id],
                                             &HYD_pmcd_pmip.downstream.pid[process_id],
                                             os_index);
                HYDU_ERR_POP(status, "create process returned error\n");

                if (HYD_pmcd_pmip.system_global.enable_stdin) {
                    stdin_fd = STDIN_FILENO;
                    status = HYDT_dmx_register_fd(1, &stdin_fd, HYD_POLLIN, NULL, stdio_cb);
                    HYDU_ERR_POP(status, "unable to register fd\n");
                }
            }
            else {
                status = HYDU_create_process(client_args, force_env, NULL,
                                             &HYD_pmcd_pmip.downstream.out[process_id],
                                             &HYD_pmcd_pmip.downstream.err[process_id],
                                             &HYD_pmcd_pmip.downstream.pid[process_id],
                                             os_index);
                HYDU_ERR_POP(status, "create process returned error\n");
            }

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
    cmd = PID_LIST;
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &cmd, sizeof(cmd), &sent, &closed);
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");
    HYDU_ASSERT(!closed, status);

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.pid,
                             HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), &sent,
                             &closed);
    HYDU_ERR_POP(status, "unable to send PID list upstream\n");
    HYDU_ASSERT(!closed, status);

  fn_spawn_complete:
    /* Everything is spawned, register the required FDs  */
    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.out, HYD_POLLIN,
                                  (void *) (size_t) STDOUT_FILENO, stdio_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.err, HYD_POLLIN,
                                  (void *) (size_t) STDERR_FILENO, stdio_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    if (pmi_ranks)
        HYDU_FREE(pmi_ranks);
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
    if (HYD_pmcd_pmip.system_global.global_core_count == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "global core count not available\n");

    if (HYD_pmcd_pmip.local.proxy_core_count == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "proxy core count not available\n");

    if (HYD_pmcd_pmip.start_pid == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "start PID not available\n");

    if (HYD_pmcd_pmip.exec_list == NULL && HYD_pmcd_pmip.user_global.ckpoint_prefix == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "no executable given and doesn't look like a restart either\n");

    /* Set default values */
    if (HYD_pmcd_pmip.user_global.binding == NULL && HYD_pmcd_pmip.local.local_binding == NULL)
        HYD_pmcd_pmip.user_global.binding = HYDU_strdup("none");

    if (HYD_pmcd_pmip.user_global.bindlib == NULL && HYDRA_DEFAULT_BINDLIB)
        HYD_pmcd_pmip.user_global.bindlib = HYDU_strdup(HYDRA_DEFAULT_BINDLIB);

    if (HYD_pmcd_pmip.user_global.ckpointlib == NULL && HYDRA_DEFAULT_CKPOINTLIB)
        HYD_pmcd_pmip.user_global.ckpointlib = HYDU_strdup(HYDRA_DEFAULT_CKPOINTLIB);

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
    status = HYDU_sock_read(fd, &num_strings, sizeof(int), &recvd, &closed,
                            HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data from upstream\n");
    HYDU_ASSERT(!closed, status);

    HYDU_MALLOC(arglist, char **, (num_strings + 1) * sizeof(char *), status);

    for (i = 0; i < num_strings; i++) {
        status = HYDU_sock_read(fd, &str_len, sizeof(int), &recvd, &closed,
                                HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC(arglist[i], char *, str_len, status);

        status = HYDU_sock_read(fd, arglist[i], str_len, &recvd, &closed,
                                HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);
    }
    arglist[num_strings] = NULL;

    /* Get the parser to fill in the proxy params structure. */
    status = parse_exec_params(arglist);
    HYDU_ERR_POP(status, "unable to parse argument list\n");

    HYDU_free_strlist(arglist);
    HYDU_FREE(arglist);

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
    enum HYD_pmcd_pmi_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a command from upstream */
    status = HYDU_sock_read(fd, &cmd, sizeof(cmd), &cmd_len, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading command from launcher\n");

    if (closed) {
        /* The connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(fd);

        HYD_pmcd_pmip_killjob();
        status = HYD_SUCCESS;
    }
    else if (cmd == PROC_INFO) {
        status = procinfo(fd);
        HYDU_ERR_POP(status, "error parsing process info\n");

        status = launch_procs();
        HYDU_ERR_POP(status, "launch_procs returned error\n");
    }
    else if (cmd == CKPOINT) {
        HYD_pmcd_pmi_proxy_dump(status, STDOUT_FILENO, "requesting checkpoint\n");
        status = HYDT_ckpoint_suspend();
        HYDU_ERR_POP(status, "checkpoint suspend failed\n");
        HYD_pmcd_pmi_proxy_dump(status, STDOUT_FILENO, "checkpoint completed\n");
    }
    else if (cmd == PMI_RESPONSE) {
        status = handle_pmi_response(fd);
        HYDU_ERR_POP(status, "unable to handle PMI response\n");
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
