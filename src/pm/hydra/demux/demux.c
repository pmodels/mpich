/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "demux.h"

static int num_cb_fds = 0;

typedef struct HYD_DMXI_Callback {
    int num_fds;
    int *fd;
    HYD_Event_t events;
     HYD_Status(*callback) (int fd, HYD_Event_t events);

    struct HYD_DMXI_Callback *next;
} HYD_DMXI_Callback_t;

static HYD_DMXI_Callback_t *cb_list = NULL;

HYD_Status HYD_DMX_Register_fd(int num_fds, int *fd, HYD_Event_t events,
                               HYD_Status(*callback) (int fd, HYD_Event_t events))
{
    HYD_DMXI_Callback_t *cb_element, *run;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; i < num_fds; i++) {
        if (fd[i] < 0) {
            HYDU_Error_printf("registering bad fd %d\n", fd[i]);
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
    }

    HYDU_MALLOC(cb_element, HYD_DMXI_Callback_t *, sizeof(HYD_DMXI_Callback_t), status);
    cb_element->num_fds = num_fds;
    HYDU_MALLOC(cb_element->fd, int *, num_fds * sizeof(int), status);
    memcpy(cb_element->fd, fd, num_fds * sizeof(int));
    cb_element->events = events;
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


HYD_Status HYD_DMX_Deregister_fd(int fd)
{
    int i;
    HYD_DMXI_Callback_t *cb_element;
    HYD_Status status = HYD_SUCCESS;

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
    HYDU_Error_printf("couldn't find the fd to deregister: %d\n", fd);
    status = HYD_INTERNAL_ERROR;
    goto fn_fail;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_DMX_Wait_for_event(int time)
{
    int total_fds, i, j, events, ret;
    HYD_DMXI_Callback_t *run;
    struct pollfd *pollfds = NULL;
    HYD_Status status = HYD_SUCCESS;

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
            if (run->events & HYD_STDOUT)
                pollfds[i].events |= POLLIN;
            if (run->events & HYD_STDIN)
                pollfds[i].events |= POLLOUT;

            i++;
        }
        run = run->next;
    }
    total_fds = i;

    while (1) {
        ret = poll(pollfds, total_fds, time);
        if (ret < 0) {
            if (errno == EINTR) {
                /* We were interrupted by a system call; this is not
                 * an error case in the regular sense; but the upper
                 * layer needs to gracefully cleanup the processes. */
                status = HYD_SUCCESS;
                goto fn_exit;
            }
            HYDU_Error_printf("poll error (errno: %d)\n", errno);
            status = HYD_SOCK_ERROR;
            goto fn_fail;
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
                if (pollfds[i].revents & POLLOUT)
                    events |= HYD_STDIN;
                if (pollfds[i].revents & POLLIN)
                    events |= HYD_STDOUT;

                status = run->callback(pollfds[i].fd, events);
                if (status != HYD_SUCCESS) {
                    HYDU_Error_printf("callback returned error status\n", errno);
                    goto fn_fail;
                }
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


HYD_Status HYD_DMX_Finalize(void)
{
    HYD_DMXI_Callback_t *run1, *run2;
    HYD_Status status = HYD_SUCCESS;

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
