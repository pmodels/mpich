/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "external.h"

HYD_status HYDT_bscd_external_query_env_inherit(const char *env_name, int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh") || !strcmp(HYDT_bsci_info.bootstrap, "rsh") ||
        !strcmp(HYDT_bsci_info.bootstrap, "fork")) {
        if (!strncmp(env_name, "DISPLAY=", strlen("DISPLAY=")))
            *ret = 0;
        else
            *ret = 1;
    }
    else if (!strcmp(HYDT_bsci_info.bootstrap, "slurm")) {
        if (!strncmp(env_name, "SLURM", strlen("SLURM")))
            *ret = 0;
        else
            *ret = 1;
    }

    HYDU_FUNC_EXIT();

    return status;
}
