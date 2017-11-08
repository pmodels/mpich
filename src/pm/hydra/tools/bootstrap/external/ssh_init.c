/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

int HYDT_bscd_ssh_limit;
int HYDT_bscd_ssh_limit_time;
int HYDT_bscd_ssh_warnings;

HYD_status HYDT_bsci_launcher_ssh_init(void)
{
    int rc;

    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_ssh_query_env_inherit;
    HYDT_bsci_fns.launch_procs = HYDT_bscd_common_launch_procs;
    HYDT_bsci_fns.launcher_finalize = HYDT_bscd_ssh_launcher_finalize;

    rc = MPL_env2int("HYDRA_LAUNCHER_SSH_LIMIT", &HYDT_bscd_ssh_limit);
    if (rc == 0)
        HYDT_bscd_ssh_limit = HYDRA_LAUNCHER_SSH_DEFAULT_LIMIT;

    rc = MPL_env2int("HYDRA_LAUNCHER_SSH_LIMIT_TIME", &HYDT_bscd_ssh_limit_time);
    if (rc == 0)
        HYDT_bscd_ssh_limit_time = HYDRA_LAUNCHER_SSH_DEFAULT_LIMIT_TIME;

    rc = MPL_env2bool("HYDRA_LAUNCHER_SSH_ENABLE_WARNINGS", &HYDT_bscd_ssh_warnings);
    if (rc == 0)
        HYDT_bscd_ssh_warnings = 1;

    return HYD_SUCCESS;
}
