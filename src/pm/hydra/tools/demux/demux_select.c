/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "demux_internal.h"

#undef FD_SETSIZE
#define FD_SETSIZE 65536

HYD_status HYDT_dmxu_select_wait_for_event(int wtime)
{
    fd_set readfds, writefds;
    int nfds, i, ret, work_done, events;
    struct timeval timeout;
    struct HYDT_dmxu_callback *run;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);

    if (wtime >= 0)
        timeout.tv_sec = wtime;
    timeout.tv_usec = 0;

    nfds = 0;
    for (run = HYDT_dmxu_cb_list; run; run = run->next) {
        for (i = 0; i < run->num_fds; i++) {
            if (run->fd[i] == HYD_FD_UNSET)
                continue;

            if (run->events & HYD_POLLIN)
                FD_SET(run->fd[i], &readfds);
            if (run->events & HYD_POLLOUT)
                FD_SET(run->fd[i], &writefds);

            if (nfds <= run->fd[i])
                nfds = run->fd[i] + 1;
        }
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
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "select error (%s)\n",
                            HYDU_strerror(errno));
    }

    work_done = 0;
    for (run = HYDT_dmxu_cb_list; run; run = run->next) {
        for (i = 0; i < run->num_fds; i++) {
            if (run->fd[i] == HYD_FD_UNSET)
                continue;

            events = 0;
            if (FD_ISSET(run->fd[i], &readfds))
                events |= HYD_POLLIN;
            if (FD_ISSET(run->fd[i], &writefds))
                events |= HYD_POLLOUT;

            if (!events)
                continue;

            if (run->callback == NULL)
                HYDU_ERR_POP(status, "no registered callback found for socket\n");

            status = run->callback(run->fd[i], events, run->userp);
            HYDU_ERR_POP(status, "callback returned error status\n");

            work_done = 1;
        }
    }

    /* If no work has been done, it must be a timeout */
    if (!work_done)
        status = HYD_TIMED_OUT;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_dmxu_select_stdin_valid(int *out)
{
    fd_set exceptfds;
    int ret;
    struct timeval zero = { 0, 0 };
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_dmxi_stdin_valid(out);
    HYDU_ERR_POP(status, "unable to check if stdin is valid\n");

    if (*out) {
        /* The generic test thinks that STDIN is valid. Try select
         * specific tests. */

        FD_ZERO(&exceptfds);
        FD_SET(STDIN_FILENO, &exceptfds);

        /* Check if select on stdin returns any errors */
        ret = select(STDIN_FILENO + 1, NULL, NULL, &exceptfds, &zero);
        HYDU_ASSERT((ret >= 0), status);

        if (FD_ISSET(STDIN_FILENO, &exceptfds))
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
