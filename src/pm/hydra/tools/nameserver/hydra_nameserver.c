/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "demux.h"

struct HYDT_ns_publish {
    char *name;
    char *info;
    struct HYDT_ns_publish *next;
};

static struct HYDT_ns_publish *publish_list = NULL;

static struct {
    int port;
    int debug;
} private;

static void init_params(void)
{
    private.port = -1;
    private.debug = -1;
}

static void port_help_fn(void)
{
    printf("\n");
    printf("-port: Use this port to listen for requests\n\n");
}

static HYD_status port_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &private.port, atoi(**argv));

    (*argv)++;

    return status;
}

static void debug_help_fn(void)
{
    printf("\n");
    printf("-debug: Prints additional debug information\n");
}

static HYD_status debug_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, &private.debug, 1);
}

static struct HYD_arg_match_table match_table[] = {
    {"port", port_fn, port_help_fn},
    {"debug", debug_fn, debug_help_fn}
};

static void free_publish_element(struct HYDT_ns_publish *publish)
{
    if (publish == NULL)
        return;

    if (publish->name)
        MPL_free(publish->name);

    if (publish->info)
        MPL_free(publish->info);

    MPL_free(publish);
}

static HYD_status cmd_response(int fd, const char *str)
{
    int len = strlen(str) + 1;  /* include the end of string */
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_write(fd, &len, sizeof(int), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error sending publish info\n");
    HYDU_ASSERT(!closed, status);

    if (len) {
        status = HYDU_sock_write(fd, str, len, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error sending publish info\n");
        HYDU_ASSERT(!closed, status);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status request_cb(int fd, HYD_event_t events, void *userp)
{
    int len, recvd, closed, list_len;
    char *cmd, *name;
    struct HYDT_ns_publish *publish, *tmp;
    int success = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &list_len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data\n");

    if (closed) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "error deregistering fd\n");
        goto fn_exit;
    }

    status = HYDU_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data\n");
    HYDU_ASSERT(!closed, status);

    HYDU_MALLOC_OR_JUMP(cmd, char *, len + 1, status);
    status = HYDU_sock_read(fd, cmd, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data\n");
    HYDU_ASSERT(!closed, status);

    if (!strcmp(cmd, "PUBLISH")) {
        HYDU_MALLOC_OR_JUMP(publish, struct HYDT_ns_publish *, sizeof(struct HYDT_ns_publish),
                            status);

        status = HYDU_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC_OR_JUMP(publish->name, char *, len + 1, status);
        status = HYDU_sock_read(fd, publish->name, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        status = HYDU_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC_OR_JUMP(publish->info, char *, len + 1, status);
        status = HYDU_sock_read(fd, publish->info, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        publish->next = NULL;

        for (tmp = publish_list; tmp; tmp = tmp->next)
            if (!strcmp(tmp->name, publish->name))
                break;
        if (tmp == NULL)
            success = 1;

        if (publish_list == NULL)
            publish_list = publish;
        else {
            for (tmp = publish_list; tmp->next; tmp = tmp->next);
            tmp->next = publish;
        }

        if (success) {
            status = cmd_response(fd, "SUCCESS");
            HYDU_ERR_POP(status, "error responding to REMOVE request\n");
        }
        else {
            status = cmd_response(fd, "FAILURE");
            HYDU_ERR_POP(status, "error responding to REMOVE request\n");
        }
    }
    else if (!strcmp(cmd, "UNPUBLISH")) {
        status = HYDU_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC_OR_JUMP(name, char *, len + 1, status);
        status = HYDU_sock_read(fd, name, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        if (publish_list) {
            if (!strcmp(publish_list->name, name)) {
                tmp = publish_list;
                publish_list = publish_list->next;
                free_publish_element(tmp);
                success = 1;
            }
            else {
                for (tmp = publish_list; tmp->next && strcmp(tmp->next->name, name);
                     tmp = tmp->next);
                if (tmp->next) {
                    tmp->next = tmp->next->next;
                    free_publish_element(tmp->next);
                    success = 1;
                }
            }
        }

        if (success) {
            status = cmd_response(fd, "SUCCESS");
            HYDU_ERR_POP(status, "error responding to REMOVE request\n");
        }
        else {
            status = cmd_response(fd, "FAILURE");
            HYDU_ERR_POP(status, "error responding to REMOVE request\n");
        }
    }
    else if (!strcmp(cmd, "LOOKUP")) {
        status = HYDU_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        HYDU_MALLOC_OR_JUMP(name, char *, len + 1, status);
        status = HYDU_sock_read(fd, name, len, &recvd, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data\n");
        HYDU_ASSERT(!closed, status);

        for (tmp = publish_list; tmp && strcmp(tmp->name, name); tmp = tmp->next);
        if (tmp) {
            status = cmd_response(fd, tmp->info);
            HYDU_ERR_POP(status, "error sending command response\n");
            success = 1;
        }
        else {
            status = cmd_response(fd, "");
            HYDU_ERR_POP(status, "error sending command response\n");
        }
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized command: %s\n", cmd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status listen_cb(int fd, HYD_event_t events, void *userp)
{
    int client_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a connection */
    status = HYDU_sock_accept(fd, &client_fd);
    HYDU_ERR_POP(status, "accept error\n");

    /* Register the accepted socket with the demux engine */
    status = HYDT_dmx_register_fd(1, &client_fd, HYD_POLLIN, NULL, request_cb);
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
    int listen_fd;
    HYD_status status = HYD_SUCCESS;

    status = HYDU_dbg_init("nameserver");
    HYDU_ERR_POP(status, "unable to initialize debugging\n");

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
    if (private.debug == -1 && MPL_env2bool("HYDRA_NAMESERVER_DEBUG", &private.debug) == 0)
        private.debug = 0;

    if (private.port == -1 && MPL_env2int("HYDRA_NAMESERVER_PORT", &private.port) == 0)
        private.port = HYDRA_NAMESERVER_DEFAULT_PORT;

    status = HYDT_dmx_init();
    HYDU_ERR_POP(status, "unable to initialize the demux engine\n");

    /* wait for connection requests and process them */
    port = (uint16_t) private.port;
    status = HYDU_sock_listen(&listen_fd, NULL, &port);
    HYDU_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYDT_dmx_register_fd(1, &listen_fd, HYD_POLLIN, NULL, listen_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    while (1) {
        /* Wait for some event to occur */
        status = HYDT_dmx_wait_for_event(-1);
        HYDU_ERR_POP(status, "demux engine error waiting for event\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
