/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_demux.h"
#include "hydra_demux_internal.h"
#include "hydra_err.h"

#undef FD_SETSIZE
#define FD_SETSIZE 65536

HYD_status HYDI_dmx_select_wait_for_event(int wtime)
{
    fd_set readfds, writefds;
    int nfds, ret, work_done, events;
    struct timeval timeout;
    struct HYDI_dmx_callback *run, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if (wtime >= 0)
        timeout.tv_sec = wtime;
    timeout.tv_usec = 0;

    nfds = 0;
    MPL_HASH_ITER(hh, HYDI_dmx_cb_list, run, tmp) {
        if (run->events & HYD_DMX_POLLIN)
            FD_SET(run->fd, &readfds);
        if (run->events & HYD_DMX_POLLOUT)
            FD_SET(run->fd, &writefds);

        if (nfds <= run->fd)
            nfds = run->fd + 1;
    }

    ret = select(nfds, &readfds, &writefds, NULL, (wtime < 0) ? NULL : &timeout);
    if (ret < 0) {
        if (errno == EINTR) {
            /* We were interrupted by a system call; this is not an
             * error case in the regular sense; but the upper layer
             * needs to gracefully cleanup the processes. */
            status = HYD_SUCCESS;
            goto fn_exit;
        }
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "select error (%s)\n", MPL_strerror(errno));
    }

    work_done = 0;
    MPL_HASH_ITER(hh, HYDI_dmx_cb_list, run, tmp) {
        events = 0;
        if (FD_ISSET(run->fd, &readfds))
            events |= HYD_DMX_POLLIN;
        if (FD_ISSET(run->fd, &writefds))
            events |= HYD_DMX_POLLOUT;

        if (!events)
            continue;

        if (run->callback == NULL)
            HYD_ERR_POP(status, "no registered callback found for socket\n");

        status = run->callback(run->fd, events, run->userp);
        HYD_ERR_POP(status, "callback returned error status\n");

        work_done = 1;
    }

    /* If no work has been done, it must be a timeout */
    if (!work_done)
        status = HYD_ERR_TIMED_OUT;

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
