/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "sge_rmk.h"

int HYDT_rmki_sge_detect(void)
{
    int ret;
    char *dummy = NULL;

    HYDU_FUNC_EXIT();

    if (MPL_env2str("SGE_ROOT", (const char **) &dummy) &&
        MPL_env2str("ARC", (const char **) &dummy) &&
        MPL_env2str("PE_HOSTFILE", (const char **) &dummy))
        ret = 1;
    else
        ret = 0;

    HYDU_FUNC_EXIT();
    return ret;
}
