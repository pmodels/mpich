/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

int HYDTI_bscd_in_env_list(const char *env_name, const char *env_list[])
{
    int i;

    for (i = 0; env_list[i]; i++)
        if (!strcmp(env_name, env_list[i]))
            return 1;

    return 0;
}
