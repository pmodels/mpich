/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_demux.h"
#include "hydra_demux_internal.h"
#include "hydra_mem.h"
#include "hydra_err.h"

HYD_status HYDI_dmx_poll_wait_for_event(int wtime)
{
    int total_fds, i, events, ret, work_done;
    struct HYDI_dmx_callback *run, *tmp;
    struct pollfd *pollfds = NULL;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    HYD_MALLOC(pollfds, struct pollfd *, HYDI_dmx_num_cb_fds * sizeof(struct pollfd), status);

    i = 0;
    MPL_HASH_ITER(hh, HYDI_dmx_cb_list, run, tmp) {
        pollfds[i].fd = run->fd;

        pollfds[i].events = 0;
        if (run->events & HYD_DMX_POLLIN)
            pollfds[i].events |= POLLIN;
        if (run->events & HYD_DMX_POLLOUT)
            pollfds[i].events |= POLLOUT;

        i++;
    }
    total_fds = i;

    /* Convert user specified time to milliseconds for poll */
    ret = poll(pollfds, total_fds, (wtime < 0) ? wtime : (wtime * 1000));
    if (ret < 0) {
        if (errno == EINTR) {
            /* We were interrupted by a system call; this is not an
             * error case in the regular sense; but the upper layer
             * needs to gracefully cleanup the processes. */
            status = HYD_SUCCESS;
            goto fn_exit;
        }
        HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "poll error (%s)\n", MPL_strerror(errno));
    }

    work_done = 0;
    i = 0;
    MPL_HASH_ITER(hh, HYDI_dmx_cb_list, run, tmp) {
        if (pollfds[i].revents) {
            work_done = 1;

            events = 0;
            if (pollfds[i].revents & POLLIN)
                events |= HYD_DMX_POLLIN;
            if (pollfds[i].revents & POLLOUT)
                events |= HYD_DMX_POLLOUT;
            if (pollfds[i].revents & POLLHUP)
                events |= HYD_DMX_POLLHUP;

            /* We only understand POLLIN/OUT/HUP */
            HYD_ASSERT(!(pollfds[i].revents & ~POLLIN & ~POLLOUT & ~POLLHUP & ~POLLERR), status);

            if (run->callback == NULL)
                HYD_ERR_POP(status, "no registered callback found for socket\n");

            status = run->callback(pollfds[i].fd, events, run->userp);
            HYD_ERR_POP(status, "callback returned error status\n");
        }

        i++;

        if (i == total_fds)
            break;
    }

    /* If no work has been done, it must be a timeout */
    if (!work_done)
        status = HYD_ERR_TIMED_OUT;

  fn_exit:
    if (pollfds)
        MPL_free(pollfds);
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
