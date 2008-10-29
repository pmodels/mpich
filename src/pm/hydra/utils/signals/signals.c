/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_sig.h"

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYDU_Set_signal"
HYD_Status HYDU_Set_signal(int signum, sighandler_t handler)
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

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
