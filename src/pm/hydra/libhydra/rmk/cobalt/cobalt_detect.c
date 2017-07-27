/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_rmk_cobalt.h"

int HYDI_rmk_cobalt_detect(void)
{
    int ret;
    char *dummy = NULL;

    HYD_FUNC_EXIT();

    if (MPL_env2str("COBALT_NODEFILE", (const char **) &dummy))
        ret = 1;
    else
        ret = 0;

    HYD_FUNC_EXIT();
    return ret;
}
