/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "topo.h"
#include "persist_server.h"

struct HYDT_persist_handle HYDT_persist_handle;
static struct {
    enum {
        MASTER,
        SLAVE
    } type;

    /* master variables */
    int listen_fd;
    int slave_pid;

    /* client variables */
    int client_fd;
    int stdout_fd;
    int stderr_fd;
    int app_pid;
} private;

static void init_params(void)
{
    HYDT_persist_handle.port = -1;
    HYDT_persist_handle.debug = -1;
}

static void port_help_fn(void)
{
    printf("\n");
    printf("-port: Use this port to listen for requests\n\n");
}

static HYD_status port_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYDT_persist_handle.port, atoi(**argv));
    (*argv)++;

    return status;
}

static void debug_help_fn(void)
{
    printf("\n");
    printf("-debug: Prints additional debug information\n");
    printf("        In debug mode, there are two constraints:\n");
    printf("             1. only one job is allowed at a time\n");
    printf("             2. stdout is redirected to stderr\n\n");
}

static HYD_status debug_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, &HYDT_persist_handle.debug, 1);
}

static struct HYD_arg_match_table match_table[] = {
    {"port", port_fn, port_help_fn},
    {"debug", debug_fn, debug_help_fn}
};

static HYD_status stdio_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, count, sent;
    char buf[HYD_TMPBUF_SIZE];
    HYDT_persist_header hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (fd == private.stdout_fd) {
        /* stdout event */
        status = HYDU_sock_read(private.stdout_fd, buf, HYD_TMPBUF_SIZE, &count,
                                &closed, HYDU_SOCK_COMM_NONE);
        HYDU_ERR_POP(status, "error reading stdout from upstream\n");
        HYDU_ASSERT(!closed, status);

        hdr.io_type = HYDT_PERSIST_STDOUT;
        hdr.buflen = count;

        status = HYDU_sock_write(private.client_fd, &hdr, sizeof(hdr), &sent, &closed);
        HYDU_ERR_POP(status, "error sending header to client\n");
        HYDU_ASSERT(!closed, status);

        if (hdr.buflen) {
            status = HYDU_sock_write(private.client_fd, buf, count, &sent, &closed);
            HYDU_ERR_POP(status, "error sending stdout to client\n");
            HYDU_ASSERT(!closed, status);
        }
        else {
            status = HYDT_dmx_deregister_fd(private.stdout_fd);
            HYDU_ERR_SETANDJUMP(status, status, "error deregistering fd %d\n",
                                private.stdout_fd);
            close(private.stdout_fd);
        }
    }
    else if (fd == private.stderr_fd) {
        /* stderr event */
        hdr.io_type = HYDT_PERSIST_STDERR;
        hdr.buflen = 0;

        status = HYDU_sock_read(private.stderr_fd, buf, HYD_TMPBUF_SIZE, &count, &closed,
                                HYDU_SOCK_COMM_NONE);
        HYDU_ERR_POP(status, "error reading stdout from upstream\n");
        HYDU_ASSERT(!closed, status);

        hdr.io_type = HYDT_PERSIST_STDOUT;
        hdr.buflen = count;

        status = HYDU_sock_write(private.client_fd, &hdr, sizeof(hdr), &sent, &closed);
        HYDU_ERR_POP(status, "error sending header to client\n");
        HYDU_ASSERT(!closed, status);

        if (hdr.buflen) {
            status = HYDU_sock_write(private.client_fd, buf, count, &sent, &closed);
            HYDU_ERR_POP(status, "error sending stdout to client\n");
            HYDU_ASSERT(!closed, status);
        }
        else {
            status = HYDT_dmx_deregister_fd(private.stderr_fd);
            HYDU_ERR_SETANDJUMP(status, status, "error deregistering fd %d\n",
                                private.stderr_fd);
            close(private.stderr_fd);
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status listen_cb(int fd, HYD_event_t events, void *userp)
{
    int recvd, i, num_strings, str_len, closed;
    char **args;
    struct HYDT_topo_cpuset_t cpuset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a connection */
    status = HYDU_sock_accept(fd, &private.client_fd);
    HYDU_ERR_POP(status, "accept error\n");

    if (!HYDT_persist_handle.debug) {
        /* In debug mode, we don't fork a slave process, but the
         * master takes over the work of the slave. */

        /* fork and let the slave process handle this connection */
        private.slave_pid = fork();
        HYDU_ERR_CHKANDJUMP(status, private.slave_pid < 0, HYD_INTERNAL_ERROR,
                            "fork failed\n");

        if (private.slave_pid > 0) {    /* master process */
            close(private.client_fd);   /* the slave process will handle this */
            goto fn_exit;
        }
    }

    /* This is the slave process. Close and deregister the listen socket */
    private.type = SLAVE;
    status = HYDT_dmx_deregister_fd(private.listen_fd);
    HYDU_ERR_POP(status, "unable to deregister listen fd\n");
    close(private.listen_fd);

    /* Get the executable information */
    status = HYDU_sock_read(private.client_fd, &num_strings, sizeof(int), &recvd, &closed,
                            HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data from upstream\n");
    HYDU_ASSERT(!closed, status);

    HYDU_MALLOC(args, char **, (num_strings + 1) * sizeof(char *), status);

    for (i = 0; i < num_strings; i++) {
        status = HYDU_sock_read(private.client_fd, &str_len, sizeof(int), &recvd, &closed,
                                HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC(args[i], char *, str_len, status);

        status = HYDU_sock_read(private.client_fd, args[i], str_len, &recvd, &closed,
                                HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
        HYDU_ASSERT(!closed, status);
    }
    args[num_strings] = NULL;

    /* spawn process */
    HYDT_topo_cpuset_zero(&cpuset);
    status = HYDU_create_process(args, NULL, NULL, &private.stdout_fd,
                                 &private.stderr_fd, &private.app_pid, cpuset);
    HYDU_ERR_POP(status, "unable to create process\n");

    /* use the accepted connection for stdio */
    status = HYDT_dmx_register_fd(1, &private.client_fd, HYD_POLLIN, NULL, stdio_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(1, &private.stdout_fd, HYD_POLLIN, NULL, stdio_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(1, &private.stderr_fd, HYD_POLLIN, NULL, stdio_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

int main(int argc, char **argv)
{
    uint16_t port;
    int ret, pid;
    HYD_status status = HYD_SUCCESS;

    status = HYDU_dbg_init("hydserv");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

    init_params();

    argv++;
    do {
        /* get hydserv arguments */
        status = HYDU_parse_array(&argv, match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        if (!(*argv))
            break;
    } while (1);

    /* set default values for parameters */
    if (HYDT_persist_handle.debug == -1 &&
        MPL_env2bool("HYDSERV_DEBUG", &HYDT_persist_handle.debug) == 0)
        HYDT_persist_handle.debug = 0;

    if (HYDT_persist_handle.debug) {
        HYDU_dump_noprefix(stdout, "Running in debug mode\n");
        HYDU_dump_noprefix(stdout, "   1. Only one job will run at a time\n");
        HYDU_dump_noprefix(stdout, "   2. Stdout will be redirected to stderr\n");
        HYDU_dump_noprefix(stdout, "\n");
    }

    if (HYDT_persist_handle.port == -1 &&
        MPL_env2int("HYDSERV_PORT", &HYDT_persist_handle.port) == 0)
        HYDT_persist_handle.port = PERSIST_DEFAULT_PORT;

    /* wait for connection requests and process them */
    port = (uint16_t) HYDT_persist_handle.port;
    status = HYDU_sock_listen(&private.listen_fd, NULL, &port);
    HYDU_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYDT_dmx_register_fd(1, &private.listen_fd, HYD_POLLIN, NULL, listen_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* set type to master; when a slave forks out, it'll reset the
     * type to slave. */
    private.type = MASTER;

    while (1) {
        /* Wait for some event to occur */
        status = HYDT_dmx_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");

        if (private.type == SLAVE) {
            /* check if all stdio fd's have been deregistered */

            if (HYDT_dmx_query_fd_registration(private.stdout_fd) ||
                HYDT_dmx_query_fd_registration(private.stderr_fd) ||
                HYDT_dmx_query_fd_registration(private.client_fd))
                continue;

            /* wait for the app process to terminate */
            pid = waitpid(-1, &ret, WNOHANG);
            if (pid > 0) {
                if (pid != private.app_pid)
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                        "waitpid returned incorrect pid\n");
            }

            break;
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
