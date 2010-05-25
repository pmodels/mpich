/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "hydra_utils.h"
#include "ckpoint.h"

#if defined HAVE_BLCR
#include "blcr/ckpoint_blcr.h"
#endif /* HAVE_BLCR */

struct HYDT_ckpoint_info HYDT_ckpoint_info;

HYD_status HYDT_ckpoint_init(char *user_ckpointlib, char *user_ckpoint_prefix)
{
    HYD_status status = HYD_SUCCESS;
    int ret;
    struct stat st;

    HYDU_FUNC_ENTER();

    if (user_ckpointlib)
        HYDT_ckpoint_info.ckpointlib = user_ckpointlib;
    else
        HYD_GET_ENV_STR_VAL(HYDT_ckpoint_info.ckpointlib, "HYDRA_CKPOINTLIB",
                            HYDRA_DEFAULT_CKPOINTLIB);

    if (user_ckpoint_prefix)
        HYDT_ckpoint_info.ckpoint_prefix = user_ckpoint_prefix;
    else
        HYD_GET_ENV_STR_VAL(HYDT_ckpoint_info.ckpoint_prefix, "HYDRA_CKPOINT_PREFIX", NULL);

    if (HYDT_ckpoint_info.ckpoint_prefix == NULL)
        goto fn_exit;

    ret = stat(HYDT_ckpoint_info.ckpoint_prefix, &st);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE, "Failed to stat checkpoint prefix \"%s\": %s\n", HYDT_ckpoint_info.ckpoint_prefix, strerror(errno));
    HYDU_ERR_CHKANDJUMP(status, !S_ISDIR(st.st_mode), HYD_FAILURE, "checkpoint prefix \"%s\" is not a directory.\n", HYDT_ckpoint_info.ckpoint_prefix);

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDT_ckpoint_blcr_init();
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ckpoint_suspend(int pgid, int id)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ERR_CHKANDJUMP(status, HYDT_ckpoint_info.ckpoint_prefix == NULL, HYD_INTERNAL_ERROR,
                        "no checkpoint prefix defined\n");

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status = HYDT_ckpoint_blcr_suspend(HYDT_ckpoint_info.ckpoint_prefix, pgid, id);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ckpoint_restart(int pgid, int id, struct HYD_env *envlist, int num_ranks, int ranks[], int *in, int *out, int *err)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_ERR_CHKANDJUMP(status, HYDT_ckpoint_info.ckpoint_prefix == NULL, HYD_INTERNAL_ERROR,
                        "no checkpoint prefix defined\n");

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status =
            HYDT_ckpoint_blcr_restart(HYDT_ckpoint_info.ckpoint_prefix, pgid, id, envlist, num_ranks, ranks, in, out, err);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
