/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"

static int num_cb_fds = 0;

struct dmx_callback {
    int num_fds;
    int *fd;
    HYD_event_t events;
    void *userp;
     HYD_status(*callback) (int fd, HYD_event_t events, void *userp);

    struct dmx_callback *next;
};

static struct dmx_callback *cb_list = NULL;

HYD_status HYDU_dmx_register_fd(int num_fds, int *fd, HYD_event_t events, void *userp,
                                HYD_status(*callback) (int fd, HYD_event_t events,
                                                       void *userp))
{
    struct dmx_callback *cb_element, *run;
#if defined HAVE_ERROR_CHECKING
    int i, j;
#endif /* HAVE_ERROR_CHECKING */
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(events, status);

#if defined HAVE_ERROR_CHECKING
    for (i = 0; i < num_fds; i++) {
        if (fd[i] < 0)
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "registering bad fd %d\n", fd[i]);

        cb_element = cb_list;
        while (cb_element) {
            for (j = 0; j < cb_element->num_fds; j++) {
                if (cb_element->fd[j] == fd[i]) {
                    HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                         "registering duplicate fd %d\n", fd[i]);
                }
            }
            cb_element = cb_element->next;
        }
    }
#endif /* HAVE_ERROR_CHECKING */

    HYDU_MALLOC(cb_element, struct dmx_callback *, sizeof(struct dmx_callback), status);
    cb_element->num_fds = num_fds;
    HYDU_MALLOC(cb_element->fd, int *, num_fds * sizeof(int), status);
    memcpy(cb_element->fd, fd, num_fds * sizeof(int));
    cb_element->events = events;
    cb_element->userp = userp;
    cb_element->callback = callback;
    cb_element->next = NULL;

    if (cb_list == NULL) {
        cb_list = cb_element;
    }
    else {
        run = cb_list;
        while (run->next)
            run = run->next;
        run->next = cb_element;
    }

    num_cb_fds += num_fds;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_dmx_deregister_fd(int fd)
{
    int i;
    struct dmx_callback *cb_element;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cb_element = cb_list;
    while (cb_element) {
        for (i = 0; i < cb_element->num_fds; i++) {
            if (cb_element->fd[i] == fd) {
                cb_element->fd[i] = -1;
                num_cb_fds--;
                goto fn_exit;
            }
        }
        cb_element = cb_element->next;
    }

    /* FD is not found */
    HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                         "could not find fd to deregister: %d\n", fd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_dmx_wait_for_event(int wtime)
{
    int total_fds, i, j, events, ret;
    struct dmx_callback *run;
    struct pollfd *pollfds = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(pollfds, struct pollfd *, num_cb_fds * sizeof(struct pollfd), status);

    run = cb_list;
    i = 0;
    while (run) {
        for (j = 0; j < run->num_fds; j++) {
            if (run->fd[j] == -1)
                continue;

            pollfds[i].fd = run->fd[j];

            pollfds[i].events = 0;
            if (run->events & HYD_POLLIN)
                pollfds[i].events |= POLLIN;
            if (run->events & HYD_POLLOUT)
                pollfds[i].events |= POLLOUT;

            i++;
        }
        run = run->next;
    }
    total_fds = i;

    while (1) {
        /* Convert user specified time to milliseconds for poll */
        ret = poll(pollfds, total_fds, (wtime < 0) ? wtime : (wtime * 1000));
        if (ret < 0) {
            if (errno == EINTR) {
                /* We were interrupted by a system call; this is not
                 * an error case in the regular sense; but the upper
                 * layer needs to gracefully cleanup the processes. */
                status = HYD_SUCCESS;
                goto fn_exit;
            }
            HYDU_ERR_SETANDJUMP1(status, HYD_SOCK_ERROR, "poll error (%s)\n",
                                 HYDU_strerror(errno));
        }
        break;
    }

    run = cb_list;
    i = 0;
    while (run) {
        for (j = 0; j < run->num_fds; j++) {
            if (run->fd[j] == -1)
                continue;

            if (pollfds[i].revents) {
                events = 0;
                if (pollfds[i].revents & POLLIN)
                    events |= HYD_POLLIN;
                if (pollfds[i].revents & POLLOUT)
                    events |= HYD_POLLOUT;

                /* We only understand POLLIN/OUT/HUP */
                HYDU_ASSERT(!(pollfds[i].revents & ~events & ~POLLHUP), status);

                if (run->callback == NULL)
                    HYDU_ERR_POP(status, "no registered callback found for socket\n");

                status = run->callback(pollfds[i].fd, events, run->userp);
                HYDU_ERR_POP(status, "callback returned error status\n");
            }

            i++;
            if (i == total_fds)
                break;
        }
        run = run->next;

        if (i == total_fds)
            break;
    }

  fn_exit:
    if (pollfds)
        HYDU_FREE(pollfds);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_dmx_query_fd_registration(int fd, int *ret)
{
    struct dmx_callback *run;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *ret = 0;
    for (run = cb_list; run; run = run->next) {
        for (i = 0; i < run->num_fds; i++) {
            if (run->fd[i] == fd) {     /* found it */
                *ret = 1;
                break;
            }
        }
        if (*ret)
            break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYDU_dmx_finalize(void)
{
    struct dmx_callback *run1, *run2;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    run1 = cb_list;
    while (run1) {
        run2 = run1->next;
        if (run1->fd)
            HYDU_FREE(run1->fd);
        HYDU_FREE(run1);
        run1 = run2;
    }
    cb_list = NULL;

    HYDU_FUNC_EXIT();
    return status;
}

static int stdin_valid = -1;

int HYDU_dmx_stdin_valid(void)
{
    struct pollfd fd[1];
    int ret;

    HYDU_FUNC_ENTER();

    if (stdin_valid != -1)
        return stdin_valid;

    fd[0].fd = STDIN_FILENO;
    fd[0].events = POLLIN;

    /* Check if poll on stdin returns any errors; on Darwin this is
     * broken.
     *
     * FIXME: Should we specifically check for POLLNVAL or any
     * error? */
    ret = poll(fd, 1, 0);
    if (ret < 0)
        stdin_valid = 0;
    else
        stdin_valid = 1;

    return stdin_valid;
}
