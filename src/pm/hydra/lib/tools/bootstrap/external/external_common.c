/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

int HYDTI_bscd_env_is_avail(const char *env_name)
{
    char *dummy = NULL;

    return MPL_env2str(env_name, (const char **) &dummy);
}

int HYDTI_bscd_in_env_list(const char *env_name, const char *env_list[])
{
    int i;

    for (i = 0; env_list[i]; i++)
        if (!strcmp(env_name, env_list[i]))
            return 1;

    return 0;
}
