/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "demux_internal.h"

HYD_status HYDT_dmxu_poll_wait_for_event(int wtime)
{
    int total_fds, i, j, events, ret, work_done;
    struct HYDT_dmxu_callback *run;
    struct pollfd *pollfds = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(pollfds, struct pollfd *, HYDT_dmxu_num_cb_fds * sizeof(struct pollfd), status);

    for (i = 0, run = HYDT_dmxu_cb_list; run; run = run->next) {
        for (j = 0; j < run->num_fds; j++) {
            if (run->fd[j] == HYD_FD_UNSET)
                continue;

            pollfds[i].fd = run->fd[j];

            pollfds[i].events = 0;
            if (run->events & HYD_POLLIN)
                pollfds[i].events |= POLLIN;
            if (run->events & HYD_POLLOUT)
                pollfds[i].events |= POLLOUT;

            i++;
        }
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
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "poll error (%s)\n", HYDU_strerror(errno));
    }

    work_done = 0;
    for (i = 0, run = HYDT_dmxu_cb_list; run; run = run->next) {
        for (j = 0; j < run->num_fds; j++) {
            if (run->fd[j] == HYD_FD_UNSET)
                continue;

            if (pollfds[i].revents) {
                work_done = 1;

                events = 0;
                if (pollfds[i].revents & POLLIN)
                    events |= HYD_POLLIN;
                if (pollfds[i].revents & POLLOUT)
                    events |= HYD_POLLOUT;
                if (pollfds[i].revents & POLLHUP)
                    events |= HYD_POLLHUP;

                /* We only understand POLLIN/OUT/HUP */
                HYDU_ASSERT(!(pollfds[i].revents & ~POLLIN & ~POLLOUT & ~POLLHUP & ~POLLERR),
                            status);

                if (run->callback == NULL)
                    HYDU_ERR_POP(status, "no registered callback found for socket\n");

                status = run->callback(pollfds[i].fd, events, run->userp);
                HYDU_ERR_POP(status, "callback returned error status\n");
            }

            i++;
            if (i == total_fds)
                break;
        }

        if (i == total_fds)
            break;
    }

    /* If no work has been done, it must be a timeout */
    if (!work_done)
        status = HYD_TIMED_OUT;

  fn_exit:
    if (pollfds)
        HYDU_FREE(pollfds);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_dmxu_poll_stdin_valid(int *out)
{
    struct pollfd fd[1];
    int ret;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_dmxi_stdin_valid(out);
    HYDU_ERR_POP(status, "unable to check if stdin is valid\n");

    if (*out) {
        /* The generic test thinks that STDIN is valid. Try poll
         * specific tests. */

        fd[0].fd = STDIN_FILENO;
        fd[0].events = POLLIN;

        /* Check if poll on stdin returns any errors; on Darwin this
         * is broken */
        ret = poll(fd, 1, 0);
        HYDU_ASSERT((ret >= 0), status);

        if (fd[0].revents & ~(POLLIN | POLLHUP))
            *out = 0;
        else
            *out = 1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
