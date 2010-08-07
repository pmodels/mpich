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

    if (!strncmp(env_name, "DISPLAY=", strlen("DISPLAY=")))
        *ret = 0;
    else
        *ret = 1;

    if (!strcmp(HYDT_bsci_info.bootstrap, "lsf")) {
        if (!strncmp(env_name, "LSB_", strlen("LSB_")))
            *ret = 0;
        else
            *ret = 1;
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "sge")) {
        if (!strncmp(env_name, "PE_", strlen("PE_")))
            *ret = 0;
        else
            *ret = 1;
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "slurm")) {
        if (!strncmp(env_name, "SLURM_", strlen("SLURM_")))
            *ret = 0;
        else
            *ret = 1;
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "ll")) {
        if (!strncmp(env_name, "LOADL_", strlen("LOADL_")) ||
            !strncmp(env_name, "MP_", strlen("MP_")))
            *ret = 0;
        else
            *ret = 1;
    }

    HYDU_FUNC_EXIT();

    return status;
}
