/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "csi.h"
#include "csiu.h"
#include "pmci.h"

extern HYD_Handle handle;

HYD_Status HYD_CSI_wait_for_completion(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Make sure all the processes have terminated. The process
     * manager control device will take care of that. */
    status = HYD_PMCI_wait_for_completion();
    HYDU_ERR_POP(status, "error waiting for completion\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
