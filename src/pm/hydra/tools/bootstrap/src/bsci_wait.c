/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_wait_for_completion(int timeout, int *ncompleted, int **procs, int **exit_statuses)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.wait_for_completion) {
        status = HYDT_bsci_fns.wait_for_completion(timeout, ncompleted, procs, exit_statuses);
    }
    else {
        status = HYDT_bscu_wait_for_completion(timeout, ncompleted, procs, exit_statuses);
    }
    HYDU_ERR_POP(status, "launcher returned error waiting for completion\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
