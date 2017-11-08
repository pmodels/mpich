/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

HYD_status HYDT_bscd_slurm_query_proxy_id(int *proxy_id)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (MPL_env2int("SLURM_NODEID", proxy_id) == 0) {
        *proxy_id = -1;
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cannot find slurm proxy ID\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
