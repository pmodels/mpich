/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_slurm_query_jobid(char **jobid)
{
    const char *tmp_jobid = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    MPL_env2str("SLURM_JOBID", &tmp_jobid);

    if (tmp_jobid)
        *jobid = HYDU_strdup(tmp_jobid);
    else
        *jobid = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
