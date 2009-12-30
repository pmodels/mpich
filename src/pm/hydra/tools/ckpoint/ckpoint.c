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

struct HYDT_ckpoint_info HYDT_ckpoint_info;

HYD_status HYDT_ckpoint_init(char *ckpointlib, char *ckpoint_prefix)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_ckpoint_info.ckpointlib = ckpointlib;
    HYDT_ckpoint_info.ckpoint_prefix = ckpoint_prefix;

    if (HYDT_ckpoint_info.ckpoint_prefix == NULL)
        goto fn_exit;

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDT_ckpoint_blcr_init();
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

#if defined HAVE_BLCR
  fn_fail:
    goto fn_exit;
#endif /* HAVE_BLCR */
}

HYD_status HYDT_ckpoint_suspend(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ERR_CHKANDJUMP(status, HYDT_ckpoint_info.ckpoint_prefix == NULL, HYD_INTERNAL_ERROR,
                        "no checkpoint prefix defined\n");

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDT_ckpoint_blcr_suspend(HYDT_ckpoint_info.ckpoint_prefix);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ckpoint_restart(struct HYD_env *envlist, int num_ranks, int ranks[], int *in,
                                int *out, int *err)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ERR_CHKANDJUMP(status, HYDT_ckpoint_info.ckpoint_prefix == NULL, HYD_INTERNAL_ERROR,
                        "no checkpoint prefix defined\n");

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status =
            HYDT_ckpoint_blcr_restart(HYDT_ckpoint_info.ckpoint_prefix, envlist, num_ranks,
                                      ranks, in, out, err);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
