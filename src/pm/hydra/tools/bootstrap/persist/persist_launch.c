/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "persist_client.h"

int *HYDT_bscd_persist_control_fd = NULL;
int HYDT_bscd_persist_node_count = 0;

static HYD_status persist_cb(int fd, HYD_event_t events, void *userp)
{
    int count, closed, sent;
    char buf[HYD_TMPBUF_SIZE];
    HYDT_persist_header hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading IO type\n");
    HYDU_ASSERT(!closed, status);

    if (hdr.buflen) {
        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, &closed, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data type\n");
        HYDU_ASSERT(!closed, status);

        if (hdr.io_type == HYDT_PERSIST_STDOUT) {
            HYDU_sock_write(STDOUT_FILENO, buf, hdr.buflen, &sent, &closed);
            HYDU_ERR_POP(status, "stdout forwarding error\n");
            HYDU_ASSERT(!closed, status);
            HYDU_ASSERT(sent == hdr.buflen, status);
        }
        else {
            HYDU_sock_write(STDERR_FILENO, buf, hdr.buflen, &sent, &closed);
            HYDU_ERR_POP(status, "stderr forwarding error\n");
            HYDU_ASSERT(!closed, status);
            HYDU_ASSERT(sent == hdr.buflen, status);
        }
    }
    else {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "error deregistering fd\n");

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_persist_launch_procs(char **args, struct HYD_proxy *proxy_list,
                                          int *control_fd)
{
    struct HYD_proxy *proxy;
    int idx, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bscd_persist_node_count = 0;
    for (proxy = proxy_list; proxy; proxy = proxy->next)
        HYDT_bscd_persist_node_count++;

    for (idx = 0; args[idx]; idx++);
    args[idx + 1] = NULL;

    HYDU_MALLOC(HYDT_bscd_persist_control_fd, int *,
                HYDT_bscd_persist_node_count * sizeof(int), status);

    for (proxy = proxy_list, i = 0; proxy; proxy = proxy->next, i++) {
        args[idx] = HYDU_int_to_str(i);

        /* connect to hydserv on each node */
        status = HYDU_sock_connect(proxy->node->hostname, PERSIST_DEFAULT_PORT,
                                   &HYDT_bscd_persist_control_fd[i], 0, 0);
        HYDU_ERR_POP(status, "unable to connect to the main server\n");

        /* send information about the executable */
        status = HYDU_send_strlist(HYDT_bscd_persist_control_fd[i], args);
        HYDU_ERR_POP(status, "error sending information to hydserv\n");

        status = HYDT_dmx_register_fd(1, &HYDT_bscd_persist_control_fd[i], HYD_POLLIN, NULL,
                                      persist_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
