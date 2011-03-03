/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "ckpoint.h"

#if defined HAVE_BLCR
#include "blcr/ckpoint_blcr.h"
#endif /* HAVE_BLCR */

struct HYDT_ckpoint_info HYDT_ckpoint_info;

HYD_status HYDT_ckpoint_init(char *user_ckpointlib, int user_ckpoint_num)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (user_ckpointlib)
        HYDT_ckpoint_info.ckpointlib = user_ckpointlib;
    else if (MPL_env2str("HYDRA_CKPOINTLIB", (const char **) &HYDT_ckpoint_info.ckpointlib) ==
             0)
        HYDT_ckpoint_info.ckpointlib = HYDRA_DEFAULT_CKPOINTLIB;

    HYDT_ckpoint_info.ckpoint_num = (user_ckpoint_num == -1) ? 0 : user_ckpoint_num;

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

HYD_status HYDT_ckpoint_checkpoint(int pgid, int id, const char *user_ckpoint_prefix)
{
    HYD_status status = HYD_SUCCESS;
    struct stat st;
    int ret;
    
    HYDU_FUNC_ENTER();


    HYDU_ASSERT(user_ckpoint_prefix, status);

    ret = stat(user_ckpoint_prefix, &st);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE,
                        "Failed to stat checkpoint prefix \"%s\": %s\n",
                        user_ckpoint_prefix, strerror(errno));
    HYDU_ERR_CHKANDJUMP(status, !S_ISDIR(st.st_mode), HYD_FAILURE,
                        "checkpoint prefix \"%s\" is not a directory.\n",
                        user_ckpoint_prefix);

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status =
            HYDT_ckpoint_blcr_checkpoint(user_ckpoint_prefix, pgid, id,
                                         HYDT_ckpoint_info.ckpoint_num);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

    ++HYDT_ckpoint_info.ckpoint_num;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_ckpoint_restart(int pgid, int id, struct HYD_env *envlist, int num_ranks,
                                int ranks[], int *in, int *out, int *err, int *pid,
                                const char *user_ckpoint_prefix)
{
    HYD_status status = HYD_SUCCESS;
    struct stat st;
    int ret;

    HYDU_FUNC_ENTER();

    HYDU_ASSERT(user_ckpoint_prefix, status);
    
    ret = stat(user_ckpoint_prefix, &st);
    HYDU_ERR_CHKANDJUMP(status, ret, HYD_FAILURE,
                        "Failed to stat checkpoint prefix \"%s\": %s\n",
                        user_ckpoint_prefix, strerror(errno));
    HYDU_ERR_CHKANDJUMP(status, !S_ISDIR(st.st_mode), HYD_FAILURE,
                        "checkpoint prefix \"%s\" is not a directory.\n",
                        user_ckpoint_prefix);

#if defined HAVE_BLCR
    if (!strcmp(HYDT_ckpoint_info.ckpointlib, "blcr")) {
        status =
            HYDT_ckpoint_blcr_restart(user_ckpoint_prefix, pgid, id,
                                      HYDT_ckpoint_info.ckpoint_num, envlist, num_ranks, ranks,
                                      in, out, err, pid);
        HYDU_ERR_POP(status, "blcr checkpoint returned error\n");
    }
#endif /* HAVE_BLCR */

    /* next checkpoint number should be the one after the one we restarted from */
    ++HYDT_ckpoint_info.ckpoint_num;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
