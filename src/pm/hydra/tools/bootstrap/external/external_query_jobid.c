/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "external.h"

HYD_status HYDT_bscd_external_query_jobid(char **jobid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.rmk, "pbs"))
        MPL_env2str("PBS_JOBID", (const char **) jobid);
    else if (!strcmp(HYDT_bsci_info.rmk, "slurm"))
        MPL_env2str("SLURM_JOBID", (const char **) jobid);
    else
        *jobid = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
