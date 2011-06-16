/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

int HYDTI_bscd_env_is_avail(const char *env_name)
{
    char *dummy = NULL;

    MPL_env2str(env_name, (const char **) &dummy);
    if (!dummy)
        return 0;

    return 1;
}

int HYDTI_bscd_in_env_list(const char *env_name, const char *env_list[])
{
    int i;

    for (i = 0; env_list[i]; i++)
        if (!strcmp(env_name, env_list[i]))
            return 1;

    return 0;
}
