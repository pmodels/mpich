/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "external.h"

static int env_is_avail(const char *env_name)
{
    char *dummy = NULL;

    MPL_env2str(env_name, (const char **) &dummy);
    if (!dummy)
        return 0;

    return 1;
}

HYD_status HYDT_bscd_external_query_native_int(int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *ret = 0;

    while (!strcmp(HYDT_bsci_info.bootstrap, "lsf")) {
        if (!env_is_avail("LSF_BINDIR"))
            break;
        if (!env_is_avail("LSB_MCPU_HOSTS"))
            break;

        *ret = 1;
        goto fn_exit;
    }

    while (!strcmp(HYDT_bsci_info.bootstrap, "sge")) {
        if (!env_is_avail("SGE_ROOT"))
            break;
        if (!env_is_avail("ARC"))
            break;
        if (!env_is_avail("PE_HOSTFILE"))
            break;

        *ret = 1;
        goto fn_exit;
    }

    while (!strcmp(HYDT_bsci_info.bootstrap, "slurm")) {
        if (!env_is_avail("SLURM_NODELIST"))
            break;
        if (!env_is_avail("SLURM_JOB_CPUS_PER_NODE"))
            break;

        *ret = 1;
        goto fn_exit;
    }

    while (!strcmp(HYDT_bsci_info.bootstrap, "ll")) {
        if (!env_is_avail("LOADL_HOSTFILE"))
            break;
        if (!env_is_avail("MP_CHILD"))
            break;

        *ret = 1;
        goto fn_exit;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
