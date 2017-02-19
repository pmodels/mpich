/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "demux.h"
#include "demux_internal.h"

static int got_sigttin = 0;

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
