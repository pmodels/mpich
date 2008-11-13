/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

/*
 * HYD_BSCI_Wait_for_completion: We first wait for communication
 * events from the available processes till all connections have
 * closed. In the meanwhile, the SIGCHLD handler keeps track of all
 * the terminated processes.
 */
HYD_Status HYD_BSCI_Wait_for_completion(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCU_Wait_for_completion();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
