/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_signal.h"
#include "hydra_err.h"

HYD_status HYD_signal_set(int signum, void (*handler) (int))
{
    HYD_status status = HYD_SUCCESS;
#if defined HAVE_SIGACTION
    struct sigaction act;
#endif
    HYD_FUNC_ENTER();

#if defined HAVE_SIGACTION
    /* Get the old signal action, reset the function and if possible
     * turn off the reset-handler-to-default bit, then set the new
     * handler */
    sigaction(signum, (struct sigaction *) NULL, &act);
    act.sa_handler = (void (*)(int)) handler;
#if defined SA_RESETHAND
    /* Note that if this feature is not supported, there is a race
     * condition in the handling of signals, and the OS is
     * fundementally flawed */
    act.sa_flags = act.sa_flags & ~(SA_RESETHAND);
#endif
    sigaction(signum, &act, (struct sigaction *) NULL);
#elif defined HAVE_SIGNAL
    signal(signum, handler);
#else
#error "No signaling mechanism"
#endif

    HYD_FUNC_EXIT();
    return status;
}


HYD_status HYD_signal_set_common(void (*handler) (int))
{
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = HYD_signal_set(SIGINT, handler);
    HYD_ERR_POP(status, "unable to set SIGINT\n");

    status = HYD_signal_set(SIGQUIT, handler);
    HYD_ERR_POP(status, "unable to set SIGQUIT\n");

    status = HYD_signal_set(SIGTERM, handler);
    HYD_ERR_POP(status, "unable to set SIGTERM\n");

    status = HYD_signal_set(SIGUSR1, handler);
    HYD_ERR_POP(status, "unable to set SIGUSR1\n");

    status = HYD_signal_set(SIGALRM, handler);
    HYD_ERR_POP(status, "unable to set SIGALRM\n");

  fn_exit:
    HYD_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
