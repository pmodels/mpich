/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"

struct nameserv_publish {
    char *name;
    char *info;
    struct nameserv_publish *next;
};

static struct nameserv_publish *publish_list = NULL;

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

    status = HYD_arg_set_int(arg, &private.port, atoi(**argv));

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
    return HYD_arg_set_int(arg, &private.debug, 1);
}

static struct HYD_arg_match_table match_table[] = {
    {"port", port_fn, port_help_fn},
    {"debug", debug_fn, debug_help_fn}
};

static void free_publish_element(struct nameserv_publish *publish)
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

    HYD_FUNC_ENTER();

    status = HYD_sock_write(fd, &len, sizeof(int), &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error sending publish info\n");
    HYD_ASSERT(!closed, status);

    if (len) {
        status = HYD_sock_write(fd, str, len, &sent, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error sending publish info\n");
        HYD_ASSERT(!closed, status);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status request_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    int len, recvd, closed, list_len;
    char *cmd, *name;
    struct nameserv_publish *publish, *tmp;
    int success = 0;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status =
        HYD_sock_read(fd, &list_len, sizeof(int), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading data\n");

    if (closed) {
        status = HYD_dmx_deregister_fd(fd);
        HYD_ERR_POP(status, "error deregistering fd\n");
        goto fn_exit;
    }

    status = HYD_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading data\n");
    HYD_ASSERT(!closed, status);

    HYD_MALLOC(cmd, char *, len + 1, status);
    status = HYD_sock_read(fd, cmd, len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
    HYD_ERR_POP(status, "error reading data\n");
    HYD_ASSERT(!closed, status);

    if (!strcmp(cmd, "PUBLISH")) {
        HYD_MALLOC(publish, struct nameserv_publish *, sizeof(struct nameserv_publish), status);

        status =
            HYD_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        HYD_MALLOC(publish->name, char *, len + 1, status);
        status =
            HYD_sock_read(fd, publish->name, len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        status =
            HYD_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        HYD_MALLOC(publish->info, char *, len + 1, status);
        status =
            HYD_sock_read(fd, publish->info, len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

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
            HYD_ERR_POP(status, "error responding to REMOVE request\n");
        }
        else {
            status = cmd_response(fd, "FAILURE");
            HYD_ERR_POP(status, "error responding to REMOVE request\n");
        }
    }
    else if (!strcmp(cmd, "UNPUBLISH")) {
        status =
            HYD_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        HYD_MALLOC(name, char *, len + 1, status);
        status = HYD_sock_read(fd, name, len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

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
            HYD_ERR_POP(status, "error responding to REMOVE request\n");
        }
        else {
            status = cmd_response(fd, "FAILURE");
            HYD_ERR_POP(status, "error responding to REMOVE request\n");
        }
    }
    else if (!strcmp(cmd, "LOOKUP")) {
        status =
            HYD_sock_read(fd, &len, sizeof(int), &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        HYD_MALLOC(name, char *, len + 1, status);
        status = HYD_sock_read(fd, name, len, &recvd, &closed, HYD_SOCK_COMM_TYPE__BLOCKING);
        HYD_ERR_POP(status, "error reading data\n");
        HYD_ASSERT(!closed, status);

        for (tmp = publish_list; tmp && strcmp(tmp->name, name); tmp = tmp->next);
        if (tmp) {
            status = cmd_response(fd, tmp->info);
            HYD_ERR_POP(status, "error sending command response\n");
            success = 1;
        }
        else {
            status = cmd_response(fd, "");
            HYD_ERR_POP(status, "error sending command response\n");
        }
    }
    else {
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "unrecognized command: %s\n", cmd);
    }

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status listen_cb(int fd, HYD_dmx_event_t events, void *userp)
{
    int client_fd;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    /* We got a connection */
    status = HYD_sock_accept(fd, &client_fd);
    HYD_ERR_POP(status, "accept error\n");

    /* Register the accepted socket with the demux engine */
    status = HYD_dmx_register_fd(client_fd, HYD_DMX_POLLIN, NULL, request_cb);
    HYD_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

int main(int argc, char **argv)
{
    uint16_t port;
    int listen_fd;
    HYD_status status = HYD_SUCCESS;

    status = HYD_print_set_prefix_str("nameserver");
    HYD_ERR_POP(status, "unable to initialize debugging\n");

    init_params();

    argv++;
    do {
        /* get hydserv arguments */
        status = HYD_arg_parse_array(&argv, match_table);
        HYD_ERR_POP(status, "error parsing input array\n");

        if (!(*argv))
            break;
    } while (1);

    /* set default values for parameters */
    if (private.debug == -1 && MPL_env2bool("HYDRA_NAMESERVER_DEBUG", &private.debug) == 0)
        private.debug = 0;

    if (private.port == -1 && MPL_env2int("HYDRA_NAMESERVER_PORT", &private.port) == 0)
        private.port = HYDRA_NAMESERVER_DEFAULT_PORT;

    /* wait for connection requests and process them */
    port = (uint16_t) private.port;
    status = HYD_sock_listen_on_port(&listen_fd, port);
    HYD_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYD_dmx_register_fd(listen_fd, HYD_DMX_POLLIN, NULL, listen_cb);
    HYD_ERR_POP(status, "unable to register fd\n");

    while (1) {
        /* Wait for some event to occur */
        status = HYD_dmx_wait_for_event(-1);
        HYD_ERR_POP(status, "demux engine error waiting for event\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
