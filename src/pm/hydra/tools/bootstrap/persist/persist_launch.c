/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "persist_client.h"

static HYD_status(*out_cb) (void *buf, int buflen);
static HYD_status(*err_cb) (void *buf, int buflen);

int *HYDT_bscd_persist_control_fd = NULL;
int HYDT_bscd_persist_node_count = 0;

static HYD_status persist_cb(int fd, HYD_event_t events, void *userp)
{
    int count;
    char buf[HYD_TMPBUF_SIZE];
    HYDT_persist_header hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading IO type\n");

    if (hdr.buflen) {
        status = HYDU_sock_read(fd, buf, hdr.buflen, &count, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data type\n");

        if (hdr.io_type == HYDT_PERSIST_STDOUT) {
            status = out_cb(buf, hdr.buflen);
            HYDU_ERR_POP(status, "error calling stdout callback\n");
        }
        else {
            status = err_cb(buf, hdr.buflen);
            HYDU_ERR_POP(status, "error calling stderr callback\n");
        }
    }
    else {
        status = HYDU_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "error deregistering fd\n");

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscd_persist_launch_procs(char **args, struct HYD_node *node_list,
                                          HYD_status(*stdout_cb) (void *buf, int buflen),
                                          HYD_status(*stderr_cb) (void *buf, int buflen))
{
    struct HYD_node *node;
    int idx, i, tmp_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bscd_persist_node_count = 0;
    for (node = node_list; node; node = node->next)
        HYDT_bscd_persist_node_count++;

    for (idx = 0; args[idx]; idx++);
    args[idx + 1] = NULL;

    HYDU_MALLOC(HYDT_bscd_persist_control_fd, int *,
                HYDT_bscd_persist_node_count * sizeof(int), status);

    for (node = node_list, i = 0; node; node = node->next, i++) {
        args[idx] = HYDU_int_to_str(i);

        /* connect to hydserv on each node */
        status = HYDU_sock_connect(node->hostname, PERSIST_DEFAULT_PORT,
                                   &HYDT_bscd_persist_control_fd[i]);
        HYDU_ERR_POP(status, "unable to connect to the main server\n");

        /* send information about the executable */
        status = HYDU_send_strlist(HYDT_bscd_persist_control_fd[i], args);
        HYDU_ERR_POP(status, "error sending information to hydserv\n");

        if (i == 0) {
            tmp_fd = STDIN_FILENO;
            status = HYDU_dmx_register_fd(1, &tmp_fd, HYD_POLLIN,
                                          &HYDT_bscd_persist_control_fd[i],
                                          HYDT_bscu_stdin_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }

        status = HYDU_dmx_register_fd(1, &HYDT_bscd_persist_control_fd[i], HYD_POLLIN, NULL,
                                      persist_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

    out_cb = stdout_cb;
    err_cb = stderr_cb;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
