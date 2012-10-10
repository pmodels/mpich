/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bscu.h"

HYD_status HYDT_bscu_stdio_cb(int fd, HYD_event_t events, void *userp)
{
    int stdfd, closed, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    stdfd = (int) (size_t) userp;

    status = HYDU_sock_forward_stdio(fd, stdfd, &closed);
    HYDU_ERR_POP(status, "stdio forwarding error\n");

    if (closed || (events & HYD_POLLHUP)) {
        /* connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_SETANDJUMP(status, status, "error deregistering fd %d\n", fd);

        for (i = 0; i < HYD_bscu_fd_count; i++) {
            if (HYD_bscu_fd_list[i] == fd) {
                HYD_bscu_fd_list[i] = HYD_FD_CLOSED;
                break;
            }
        }

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
