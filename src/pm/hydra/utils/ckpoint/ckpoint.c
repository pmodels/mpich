/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "ckpoint.h"

#if defined HAVE_BLCR
#include "blcr/ckpoint_blcr.h"
#endif /* HAVE_BLCR */

struct HYDU_ckpoint_info HYDU_ckpoint_info;

HYD_Status HYDU_ckpoint_init(char *ckpointlib, char *ckpoint_prefix)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ckpoint_info.ckpointlib = ckpointlib;
    HYDU_ckpoint_info.ckpoint_prefix = ckpoint_prefix;

#if defined HAVE_BLCR
    if (!strcmp(HYDU_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDU_ckpoint_blcr_init();
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_suspend()
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_BLCR
    if (!strcmp(HYDU_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDU_ckpoint_blcr_suspend(HYDU_ckpoint_info.ckpoint_prefix);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}

HYD_Status HYDU_ckpoint_restart(int num_vars, const char **env_vars)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined HAVE_BLCR
    if (!strcmp(HYDU_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDU_ckpoint_blcr_restart(HYDU_ckpoint_info.ckpoint_prefix,
                                           num_vars, env_vars);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
