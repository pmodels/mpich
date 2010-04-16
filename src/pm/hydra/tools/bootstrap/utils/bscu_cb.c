/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bscu.h"

HYD_status HYDT_bscu_inter_cb(int fd, HYD_event_t events, void *userp)
{
    int buflen, i;
    char buf[HYD_TMPBUF_SIZE];
    HYD_status(*cb) (void *buf, int buflen);
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Get the callback information */
    cb = (HYD_status(*)(void *buf, int buflen)) userp;

    status = HYDU_sock_read(fd, buf, HYD_TMPBUF_SIZE, &buflen, HYDU_SOCK_COMM_NONE);
    HYDU_ERR_POP(status, "error reading from fd\n");

    if (buflen == 0 || (events & HYD_POLLHUP)) {
        /* connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);

        for (i = 0; i < HYD_bscu_fd_count; i++) {
            if (HYD_bscu_fd_list[i] == fd) {
                HYD_bscu_fd_list[i] = -1;
                break;
            }
        }

        close(fd);
    }

    status = cb(buf, buflen);
    HYDU_ERR_POP(status, "callback returned error\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bscu_stdin_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, in_fd, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    in_fd = *((int *) userp);

    status = HYDU_sock_forward_stdio(STDIN_FILENO, in_fd, &closed);
    HYDU_ERR_POP(status, "stdin forwarding error\n");

    if (closed || (events & HYD_POLLHUP)) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);

        for (i = 0; i < HYD_bscu_fd_count; i++) {
            if (HYD_bscu_fd_list[i] == fd) {
                HYD_bscu_fd_list[i] = -1;
                break;
            }
        }

        close(STDIN_FILENO);
        close(in_fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
