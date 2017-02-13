/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "cobalt_rmk.h"

int HYDT_rmki_cobalt_detect(void)
{
    int ret;
    char *dummy = NULL;

    HYDU_FUNC_EXIT();

    if (MPL_env2str("COBALT_NODEFILE", (const char **) &dummy))
        ret = 1;
    else
        ret = 0;

    HYDU_FUNC_EXIT();
    return ret;
}
