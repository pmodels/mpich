/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
            status =
                HYDU_sock_write(STDOUT_FILENO, buf, hdr.buflen, &sent, &closed,
                                HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "stdout forwarding error\n");
            HYDU_ASSERT(!closed, status);
            HYDU_ASSERT(sent == hdr.buflen, status);
        } else {
            status =
                HYDU_sock_write(STDERR_FILENO, buf, hdr.buflen, &sent, &closed,
                                HYDU_SOCK_COMM_MSGWAIT);
            HYDU_ERR_POP(status, "stderr forwarding error\n");
            HYDU_ASSERT(!closed, status);
            HYDU_ASSERT(sent == hdr.buflen, status);
        }
    } else {
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

/* NOTE: tree-launch not supported, params k, myid are ignored. */
HYD_status HYDT_bscd_persist_launch_procs(int pgid, char **args, struct HYD_host *hosts,
                                          int num_hosts, int use_rmk, int k, int myid,
                                          int *control_fd)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bscd_persist_node_count = num_hosts;

    char *targs[HYD_NUM_TMP_STRINGS];
    int idx = 0;
    int id_idx, host_idx;
    for (int i = 0; args[i]; i++) {
        targs[idx] = MPL_strdup(args[i]);
    }
    targs[idx++] = MPL_strdup("--hostname");
    targs[idx++] = NULL;
    host_idx = idx - 1;

    targs[idx++] = MPL_strdup("--proxy-id");
    targs[idx++] = NULL;
    id_idx = idx - 1;

    targs[idx] = NULL;

    HYDU_MALLOC_OR_JUMP(HYDT_bscd_persist_control_fd, int *,
                        HYDT_bscd_persist_node_count * sizeof(int), status);

    for (int i = 0; i < num_hosts; i++) {
        MPL_free(targs[id_idx]);
        targs[id_idx] = HYDU_int_to_str(i);

        MPL_free(targs[host_idx]);
        targs[host_idx] = MPL_strdup(hosts[i].hostname);

        /* connect to hydserv on each node */
        status = HYDU_sock_connect(hosts[i].hostname, PERSIST_DEFAULT_PORT,
                                   &HYDT_bscd_persist_control_fd[i], 0, HYD_CONNECT_DELAY);
        HYDU_ERR_POP(status, "unable to connect to the main server\n");

        /* send information about the executable */
        status = HYDU_send_strlist(HYDT_bscd_persist_control_fd[i], args);
        HYDU_ERR_POP(status, "error sending information to hydserv\n");

        status = HYDT_dmx_register_fd(1, &HYDT_bscd_persist_control_fd[i], HYD_POLLIN, NULL,
                                      persist_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

  fn_exit:
    HYDU_free_strlist(targs);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
