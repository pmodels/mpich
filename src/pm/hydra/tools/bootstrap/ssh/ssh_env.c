/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "ssh.h"

HYD_status HYDT_bscd_ssh_query_env_inherit(const char *env_name, int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(env_name, "DISPLAY"))
        *ret = 0;
    else
        *ret = 1;

    HYDU_FUNC_EXIT();

    return status;
}
