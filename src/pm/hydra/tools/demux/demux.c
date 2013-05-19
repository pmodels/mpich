/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "demux.h"
#include "demux_internal.h"

int HYDT_dmxu_num_cb_fds = 0;
struct HYDT_dmxu_callback *HYDT_dmxu_cb_list = NULL;
struct HYDT_dmxu_fns HYDT_dmxu_fns = { 0 };

static int got_sigttin = 0;
static int stdin_valid;

#if defined(SIGTTIN)
static void signal_cb(int sig)
{
    HYDU_FUNC_ENTER();

    if (sig == SIGTTIN)
        got_sigttin = 1;
    /* Ignore all other signals */

    HYDU_FUNC_EXIT();
    return;
}
#endif /* SIGTTIN */

HYD_status HYDT_dmx_init(char **demux)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!(*demux)) {    /* user didn't specify anything */
#if defined HAVE_POLL
        HYDT_dmxu_fns.wait_for_event = HYDT_dmxu_poll_wait_for_event;
        HYDT_dmxu_fns.stdin_valid = HYDT_dmxu_poll_stdin_valid;
        *demux = HYDU_strdup("poll");
#elif defined HAVE_SELECT
        HYDT_dmxu_fns.wait_for_event = HYDT_dmxu_select_wait_for_event;
        HYDT_dmxu_fns.stdin_valid = HYDT_dmxu_select_stdin_valid;
        *demux = HYDU_strdup("select");
#endif /* HAVE_SELECT */
    }
    else if (!strcmp(*demux, "poll")) { /* user wants to use poll */
#if defined HAVE_POLL
        HYDT_dmxu_fns.wait_for_event = HYDT_dmxu_poll_wait_for_event;
        HYDT_dmxu_fns.stdin_valid = HYDT_dmxu_poll_stdin_valid;
#endif /* HAVE_POLL */
    }
    else if (!strcmp(*demux, "select")) {       /* user wants to use select */
#if defined HAVE_SELECT
        HYDT_dmxu_fns.wait_for_event = HYDT_dmxu_select_wait_for_event;
        HYDT_dmxu_fns.stdin_valid = HYDT_dmxu_select_stdin_valid;
#endif /* HAVE_SELECT */
    }

    if (HYDT_dmxu_fns.wait_for_event == NULL || HYDT_dmxu_fns.stdin_valid == NULL) {
        /* We couldn't find anything; return an error */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "cannot find an appropriate demux engine\n");
    }

    status = HYDT_dmxu_fns.stdin_valid(&stdin_valid);
    HYDU_ERR_POP(status, "error checking for stdin validity\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_dmx_register_fd(int num_fds, int *fd, HYD_event_t events, void *userp,
                                HYD_status(*callback) (int fd, HYD_event_t events, void *userp))
{
    struct HYDT_dmxu_callback *cb_element, *run;
#if defined HAVE_ERROR_CHECKING
    int i, j;
#endif /* HAVE_ERROR_CHECKING */
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(events, status);

#if defined HAVE_ERROR_CHECKING
    for (i = 0; i < num_fds; i++) {
        if (fd[i] < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "registering bad fd %d\n", fd[i]);

        cb_element = HYDT_dmxu_cb_list;
        while (cb_element) {
            for (j = 0; j < cb_element->num_fds; j++) {
                if (cb_element->fd[j] == fd[i]) {
                    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                        "registering duplicate fd %d\n", fd[i]);
                }
            }
            cb_element = cb_element->next;
        }
    }
#endif /* HAVE_ERROR_CHECKING */

    HYDU_MALLOC(cb_element, struct HYDT_dmxu_callback *, sizeof(struct HYDT_dmxu_callback), status);
    cb_element->num_fds = num_fds;
    HYDU_MALLOC(cb_element->fd, int *, num_fds * sizeof(int), status);
    memcpy(cb_element->fd, fd, num_fds * sizeof(int));
    cb_element->events = events;
    cb_element->userp = userp;
    cb_element->callback = callback;
    cb_element->next = NULL;

    if (HYDT_dmxu_cb_list == NULL) {
        HYDT_dmxu_cb_list = cb_element;
    }
    else {
        run = HYDT_dmxu_cb_list;
        while (run->next)
            run = run->next;
        run->next = cb_element;
    }

    HYDT_dmxu_num_cb_fds += num_fds;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_dmx_deregister_fd(int fd)
{
    int i;
    struct HYDT_dmxu_callback *cb_element;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cb_element = HYDT_dmxu_cb_list;
    while (cb_element) {
        for (i = 0; i < cb_element->num_fds; i++) {
            if (cb_element->fd[i] == fd) {
                cb_element->fd[i] = HYD_FD_UNSET;
                HYDT_dmxu_num_cb_fds--;
                goto fn_exit;
            }
        }
        cb_element = cb_element->next;
    }

    /* FD is not found */
    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "could not find fd to deregister: %d\n", fd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_dmx_wait_for_event(int wtime)
{
    return HYDT_dmxu_fns.wait_for_event(wtime);
}

int HYDT_dmx_query_fd_registration(int fd)
{
    struct HYDT_dmxu_callback *run;
    int i, ret;

    HYDU_FUNC_ENTER();

    ret = 0;
    for (run = HYDT_dmxu_cb_list; run; run = run->next) {
        for (i = 0; i < run->num_fds; i++) {
            if (run->fd[i] == fd) {     /* found it */
                ret = 1;
                break;
            }
        }
        if (ret)
            break;
    }

    HYDU_FUNC_EXIT();

    return ret;
}

HYD_status HYDT_dmx_finalize(void)
{
    struct HYDT_dmxu_callback *run1, *run2;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    run1 = HYDT_dmxu_cb_list;
    while (run1) {
        run2 = run1->next;
        if (run1->fd)
            HYDU_FREE(run1->fd);
        HYDU_FREE(run1);
        run1 = run2;
    }
    HYDT_dmxu_cb_list = NULL;

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYDT_dmxi_stdin_valid(int *out)
{
    int ret;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* This is an extremely round-about way of solving a simple
     * problem. isatty(STDIN_FILENO) seems to return 1, even when
     * mpiexec is run in the background. So, instead of relying on
     * that, we catch SIGTTIN and ignore it. But that causes the
     * read() call to return an error (with errno == EINTR) when we
     * are not attached to the terminal. */

    /*
     * We need to allow for the following cases:
     *
     *  1. mpiexec -n 2 ./foo  --> type something on the terminal
     *     Attached to terminal, and can read from stdin
     *
     *  2. mpiexec -n 2 ./foo &
     *     Attached to terminal, but cannot read from stdin
     *
     *  3. mpiexec -n 2 ./foo < bar
     *     Not attached to terminal, but can read from stdin
     *
     *  4. mpiexec -n 2 ./foo < /dev/null
     *     Not attached to terminal, and can read from stdin
     *
     *  5. mpiexec -n 2 ./foo < bar &
     *     Not attached to terminal, but can read from stdin
     *
     *  6. mpiexec -n 2 ./foo < /dev/null &
     *     Not attached to terminal, and can read from stdin
     */

#if defined(SIGTTIN)
    status = HYDU_set_signal(SIGTTIN, signal_cb);
    HYDU_ERR_POP(status, "unable to set SIGTTIN\n");
#endif /* SIGTTIN */

    ret = read(STDIN_FILENO, NULL, 0);
    if (ret == 0)
        *out = 1;
    else
        *out = 0;

#if defined(SIGTTIN)
    status = HYDU_set_signal(SIGTTIN, SIG_IGN);
    HYDU_ERR_POP(status, "unable to set SIGTTIN\n");
#endif /* SIGTTIN */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_dmx_stdin_valid(int *out)
{
    *out = stdin_valid;

    return HYD_SUCCESS;
}
