/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_set_signal(int signum, void (*handler) (int))
{
    HYD_Status status = HYD_SUCCESS;
#if defined HAVE_SIGACTION
    struct sigaction act;
#endif
    HYDU_FUNC_ENTER();

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

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYDU_set_common_signals(void (*handler) (int))
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_set_signal(SIGINT, handler);
    HYDU_ERR_POP(status, "unable to set SIGINT\n");

    status = HYDU_set_signal(SIGQUIT, handler);
    HYDU_ERR_POP(status, "unable to set SIGQUIT\n");

    status = HYDU_set_signal(SIGTERM, handler);
    HYDU_ERR_POP(status, "unable to set SIGTERM\n");

#if defined SIGSTOP
    status = HYDU_set_signal(SIGSTOP, handler);
    HYDU_ERR_POP(status, "unable to set SIGSTOP\n");
#endif /* SIGSTOP */

#if defined SIGTSTP
    status = HYDU_set_signal(SIGTSTP, handler);
    HYDU_ERR_POP(status, "unable to set SIGSTOP\n");
#endif /* SIGTSTP */

    status = HYDU_set_signal(SIGUSR1, handler);
    HYDU_ERR_POP(status, "unable to set SIGUSR1\n");

#if defined SIGCONT
    status = HYDU_set_signal(SIGCONT, handler);
    HYDU_ERR_POP(status, "unable to set SIGCONT\n");
#endif /* SIGCONT */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
