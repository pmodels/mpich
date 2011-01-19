/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

HYD_status HYDT_bscd_pbs_query_job_id(char **job_id)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    MPL_env2str("PBS_JOBID", (const char **) job_id);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
