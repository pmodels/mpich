/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.rmk_finalize) {
        status = HYDT_bsci_fns.rmk_finalize();
        HYDU_ERR_POP(status, "RMK returned error while finalizing\n");
    }

    if (HYDT_bsci_fns.launcher_finalize) {
        status = HYDT_bsci_fns.launcher_finalize();
        HYDU_ERR_POP(status, "RMK returned error while finalizing\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
