/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "ckpoint.h"
#include "libcr.h"

HYD_Status HYDU_ckpoint_blcr_suspend(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_blcr_restart(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
