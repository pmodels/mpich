/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_fns.finalize();
    HYDU_ERR_POP(status, "bootstrap device returned error while finalizing\n");

    if (HYDT_bsci_info.bootstrap)
        HYDU_FREE(HYDT_bsci_info.bootstrap);

    if (HYDT_bsci_info.bootstrap_exec)
        HYDU_FREE(HYDT_bsci_info.bootstrap_exec);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
