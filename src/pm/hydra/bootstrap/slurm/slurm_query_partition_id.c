/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

extern HYD_Handle handle;

HYD_Status HYD_BSCD_slurm_query_partition_id(int *partition_id)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (getenv("SLURM_NODEID")) {
        *partition_id = atoi(getenv("SLURM_NODEID"));
    }
    else {
        *partition_id = -1;
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "cannot find slurm partition ID\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
