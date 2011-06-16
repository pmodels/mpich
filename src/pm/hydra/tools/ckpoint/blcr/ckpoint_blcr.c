/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "ckpoint.h"
#include "ckpoint_blcr.h"
#include <libcr.h>

static int my_callback(void *arg)
{
    int rc;

    rc = cr_checkpoint(CR_CHECKPOINT_OMIT);

    switch (rc) {
    case -CR_ETEMPFAIL:
        /* One of the processes indicated that it couldn't take the checkpoint now.  Try again later. */
        return -1;
        break;
    case -CR_EPERMFAIL:
        /* One of the processes indicated a permanent failure */
        return -1;
        break;
    case -CR_EOMITTED:
        /* This is the expected return */
        break;
    default:
        /* Something bad happened */
        return -1;
    }


    return 0;
}

static HYD_status create_env_file(const struct HYD_env *envlist, int num_ranks, int *ranks)
{
    HYD_status status = HYD_SUCCESS;
    char filename[256];
    FILE *f;
    const struct HYD_env *e;
    int ret;
    int r;

    HYDU_FUNC_ENTER();

    for (r = 0; r < num_ranks; ++r) {
        MPL_snprintf(filename, sizeof(filename), "/tmp/hydra-env-file-%d:%d", (int) getpid(),
                     ranks[r]);

        f = fopen(filename, "w");
        HYDU_ERR_CHKANDJUMP(status, f == NULL, HYD_INTERNAL_ERROR, "fopen failed: %s\n",
                            strerror(errno));

        for (e = envlist; e; e = e->next) {
            fprintf(f, "%s=%s\n", e->env_name, e->env_value);
        }

        ret = fclose(f);
        HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "fclose failed: %s\n",
                            strerror(errno));
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static int listen_fd;

static HYD_status create_stdinouterr_sock(int *port)
{
    HYD_status status = HYD_SUCCESS;
    int ret;
    struct sockaddr_in sin;
    socklen_t len;
    HYDU_FUNC_ENTER();

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    HYDU_ERR_CHKANDJUMP(status, listen_fd < 0, HYD_INTERNAL_ERROR, "socket() failed, %s\n",
                        strerror(errno));

    memset((void *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = htons(0);

    ret = bind(listen_fd, (struct sockaddr *) &sin, sizeof(sin));
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "bind() failed, %s\n",
                        strerror(errno));

    ret = listen(listen_fd, SOMAXCONN);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "listen() failed, %s\n",
                        strerror(errno));

    len = sizeof(sin);
    ret = getsockname(listen_fd, (struct sockaddr *) &sin, &len);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "getsockname() failed, %s\n",
                        strerror(errno));

    *port = ntohs(sin.sin_port);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

typedef struct sock_ident {
    int rank;
    enum { IN_SOCK, OUT_SOCK, ERR_SOCK } socktype;
    int pid;
} sock_ident_t;

/* This waits for the restarted processes to reconnect their
   stdin/out/err sockets, then sets the appropriate entries in the in
   out and err arrays.  This also gets the pids of the restarted
   processes. */
static HYD_status wait_for_stdinouterr_sockets(int num_ranks, int *ranks, int *in, int *out,
                                               int *err, int *pid)
{
    HYD_status status = HYD_SUCCESS;
    int ret;
    int fd;
    int i, c;
    sock_ident_t id;
    int num_expected_connections = num_ranks * 2;       /* wait for connections for stdout and err */
    HYDU_FUNC_ENTER();

    /* if one of the processes is rank 0, we should wait for an
     * additional connection for stdin */
    for (i = 0; i < num_ranks; ++i)
        if (ranks[i] == 0) {
            ++num_expected_connections;
            break;
        }

    for (c = 0; c < num_expected_connections; ++c) {
        size_t len;
        char *id_p;
        /* wait for a connection */
        do {
            struct sockaddr_in rmt_addr;
            socklen_t sa_len = sizeof(rmt_addr);;
            fd = accept(listen_fd, (struct sockaddr *) &rmt_addr, &sa_len);
        } while (fd && errno == EINTR);
        HYDU_ERR_CHKANDJUMP(status, fd == -1, HYD_INTERNAL_ERROR, "accept failed, %s\n",
                            strerror(errno));

        /* read the socket identifier */
        len = sizeof(id);
        id_p = (char *) &id;
        do {
            do {
                ret = read(fd, id_p, len);
            } while (ret == 0 || (ret == -1 && errno == EINTR));
            HYDU_ERR_CHKANDJUMP(status, ret == -1, HYD_INTERNAL_ERROR, "read failed, %s\n",
                                strerror(errno));
            len -= ret;
            id_p += ret;
        } while (len);

        /* determine the index for this process in the stdout/err
         * arrays */
        for (i = 0; i < num_ranks; ++i)
            if (ranks[i] == id.rank)
                break;
        HYDU_ASSERT(i < num_ranks, status);

        /* assign the fd */
        switch (id.socktype) {
        case IN_SOCK:
            HYDU_ASSERT(id.rank == 0, status);
            *in = fd;
            break;
        case OUT_SOCK:
            out[i] = fd;
            break;
        case ERR_SOCK:
            err[i] = fd;
            break;
        default:
            HYDU_ASSERT(0, status);
            break;
        }

        /* assign the pid */
        pid[i] = id.pid;
    }

    ret = close(listen_fd);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "close of listener port failed, %s\n",
                        strerror(errno));


  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}




HYD_status HYDT_ckpoint_blcr_checkpoint(const char *prefix, int pgid, int id, int ckpt_num)
{
    HYD_status status = HYD_SUCCESS;
    int ret;
    int fd;
    cr_checkpoint_args_t my_args;
    cr_checkpoint_handle_t my_handle;
    char filename[256];

    HYDU_FUNC_ENTER();

    /* build the checkpoint filename */
    MPL_snprintf(filename, sizeof(filename), "%s/context-num%d-%d-%d", prefix, ckpt_num, pgid,
                 id);

    /* remove existing checkpoint file, if any */
    (void) unlink(filename);

    /* open the checkpoint file */
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC /* | O_LARGEFILE */ , 0600);
    HYDU_ERR_CHKANDJUMP(status, fd < 0, HYD_INTERNAL_ERROR, "open failed: %s\n",
                        strerror(errno));

    cr_initialize_checkpoint_args_t(&my_args);
    my_args.cr_fd = fd;
    my_args.cr_scope = CR_SCOPE_TREE;

    /* issue the request */
    ret = cr_request_checkpoint(&my_args, &my_handle);
    if (ret < 0) {
        HYDU_ERR_CHKANDJUMP(status, errno == CR_ENOSUPPORT, HYD_INTERNAL_ERROR,
                            "cr_request_checkpoint failed, %s\n", strerror(errno));
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cr_request_checkpoint failed, %s\n",
                            strerror(errno));
    }
    /* wait for the request to complete */
    while (1) {
        ret = cr_poll_checkpoint(&my_handle, NULL);
        if (ret < 0) {
            if ((ret == CR_POLL_CHKPT_ERR_POST) && (errno == CR_ERESTARTED)) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "trying to restart in a checkpoint\n");
            }
            else if (errno == EINTR) {
                /* poll was interrupted by a signal -- retry */
            }
            else {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                    "cr_poll_checkpoint failed: %s\n", strerror(errno));
            }
        }
        else if (ret == 0) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "cr_poll_checkpoint returned 0 unexpectedly\n");
        }
        else {
            break;
        }
    }

    ret = close(my_args.cr_fd);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "close failed, %s\n",
                        strerror(errno));

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

#define STDINOUTERR_PORT_NAME "CKPOINT_STDINOUTERR_PORT"

HYD_status HYDT_ckpoint_blcr_restart(const char *prefix, int pgid, int id, int ckpt_num,
                                     struct HYD_env *envlist, int num_ranks, int ranks[],
                                     int *in, int *out, int *err, int *pid)
{
    HYD_status status = HYD_SUCCESS;
    pid_t mypid;
    int ret;
    int context_fd;
    cr_restart_handle_t cr_handle;
    cr_restart_args_t args;
    char filename[256];
    char port_str[64];
    int port;

    HYDU_FUNC_ENTER();

    mypid = getpid();

    /* create listener socket for stdin/out/err */
    status = create_stdinouterr_sock(&port);
    HYDU_ERR_POP(status, "failed to create stdin/out/err socket\n");
    MPL_snprintf(port_str, sizeof(port_str), "%d", port);
    status = HYDU_append_env_to_list(STDINOUTERR_PORT_NAME, port_str, &envlist);
    HYDU_ERR_POP(status, "failed to add to env list\n");

    status = create_env_file(envlist, num_ranks, ranks);
    if (status)
        HYDU_ERR_POP(status, "blcr restart\n");

    /* open the checkpoint file */
    MPL_snprintf(filename, sizeof(filename), "%s/context-num%d-%d-%d", prefix, ckpt_num, pgid,
                 id);
    context_fd = open(filename, O_RDONLY /* | O_LARGEFILE */);
    HYDU_ERR_CHKANDJUMP(status, context_fd < 0, HYD_INTERNAL_ERROR, "open failed, %s\n",
                        strerror(errno));

    /* ... initialize the request structure */
    cr_initialize_restart_args_t(&args);
    args.cr_fd = context_fd;
    args.cr_flags = CR_RSTRT_RESTORE_PID;

    /* ... issue the request */
    ret = cr_request_restart(&args, &cr_handle);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "cr_request_restart failed, %s\n",
                        strerror(errno));

    ret = close(context_fd);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_INTERNAL_ERROR, "close failed, %s\n",
                        strerror(errno));

    /* get fds for stdin/out/err sockets, and get pids of restarted processes */
    status = wait_for_stdinouterr_sockets(num_ranks, ranks, in, out, err, pid);
    if (status)
        HYDU_ERR_POP(status, "blcr restart\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ckpoint_blcr_init(void)
{
    HYD_status status = HYD_SUCCESS;
    int rc;
    cr_client_id_t client_id;
    cr_callback_id_t callback_id;

    HYDU_FUNC_ENTER();

    client_id = (int) cr_init();
    if (client_id < 0)
        goto fn_fail;

    callback_id = cr_register_callback(my_callback, &rc, CR_SIGNAL_CONTEXT);
    if (callback_id < 0)
        goto fn_fail;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
