/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_ssh_query_env_inherit(const char *env_name, int *ret)
{
    const char *env_list[] = { "DISPLAY", NULL };

    HYDU_FUNC_ENTER();

    *ret = !HYDTI_bscd_in_env_list(env_name, env_list);

    HYDU_FUNC_EXIT();

    return HYD_SUCCESS;
}
