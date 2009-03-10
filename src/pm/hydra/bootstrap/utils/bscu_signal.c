/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"

HYD_Status HYD_BSCU_Set_common_signals(void (*handler) (int))
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_Set_signal(SIGINT, handler);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set SIGINT signal\n");
        goto fn_fail;
    }

    status = HYDU_Set_signal(SIGQUIT, handler);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set SIGQUIT signal\n");
        goto fn_fail;
    }

    status = HYDU_Set_signal(SIGTERM, handler);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set SIGTERM signal\n");
        goto fn_fail;
    }

#if defined SIGSTOP
    status = HYDU_Set_signal(SIGSTOP, handler);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set SIGSTOP signal\n");
        goto fn_fail;
    }
#endif /* SIGSTOP */

#if defined SIGCONT
    status = HYDU_Set_signal(SIGCONT, handler);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set SIGCONT signal\n");
        goto fn_fail;
    }
#endif /* SIGCONT */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYD_BSCU_Signal_handler(int signal)
{
    HYDU_FUNC_ENTER();

    if (signal == SIGINT || signal == SIGQUIT || signal == SIGTERM
#if defined SIGSTOP
        || signal == SIGSTOP
#endif /* SIGSTOP */
#if defined SIGCONT
        || signal == SIGCONT
#endif /* SIGSTOP */
) {
        /* There's nothing we can do with the return value for now. */
        HYD_BSCI_Cleanup_procs();
        exit(-1);
    }
    else {
        /* Ignore other signals for now */
    }

    HYDU_FUNC_EXIT();
    return;
}
