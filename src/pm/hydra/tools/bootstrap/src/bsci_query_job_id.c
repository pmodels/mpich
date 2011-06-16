/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_query_jobid(char **jobid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.query_jobid) {
        status = HYDT_bsci_fns.query_jobid(jobid);
        HYDU_ERR_POP(status, "RMK returned error while querying job ID\n");
    }
    else {
        /* We don't know anything about job ID */
        *jobid = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
