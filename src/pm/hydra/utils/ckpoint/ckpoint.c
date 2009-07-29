/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "ckpoint.h"

HYD_Status HYDU_ckpoint_suspend(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_BLCR
    status = HYDU_ckpoint_blcr_suspend();
    HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
#endif /* HAVE_BLCR */

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_restart(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_BLCR
    status = HYDU_ckpoint_blcr_restart();
    HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
#endif /* HAVE_BLCR */

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
