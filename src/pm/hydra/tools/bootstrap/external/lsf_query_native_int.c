/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_lsf_query_native_int(int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *ret = 1;

    if (!HYDTI_bscd_env_is_avail("LSF_BINDIR"))
        *ret = 0;
    if (!HYDTI_bscd_env_is_avail("LSB_MCPU_HOSTS"))
        *ret = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
