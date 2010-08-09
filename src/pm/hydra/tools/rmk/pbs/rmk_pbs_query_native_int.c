/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_pbs.h"

HYD_status HYDT_rmkd_pbs_query_native_int(int *ret)
{
    const char *dummy = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    MPL_env2str("PBS_NODEFILE", &dummy);
    if (!dummy)
        *ret = 0;
    else
        *ret = 1;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
