/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_query_native_int(int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.query_native_int) {
        status = HYDT_bsci_fns.query_native_int(ret);
        HYDU_ERR_POP(status, "RMK returned error querying native integration\n");
    }
    else {
        *ret = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
