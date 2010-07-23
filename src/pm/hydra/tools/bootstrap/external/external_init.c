/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "external.h"

struct HYDT_bscd_ssh_time *HYDT_bscd_ssh_time = NULL;

static HYD_status external_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh") || !strcmp(HYDT_bsci_info.bootstrap, "rsh") ||
        !strcmp(HYDT_bsci_info.bootstrap, "fork"))
        HYDT_bsci_fns.launch_procs = HYDT_bscd_external_launch_procs;
    else if (!strcmp(HYDT_bsci_info.bootstrap, "slurm"))
        HYDT_bsci_fns.launch_procs = HYDT_bscd_slurm_launch_procs;

    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh"))
        HYDT_bsci_fns.finalize = HYDT_bscd_external_finalize;

    if (!strcmp(HYDT_bsci_info.bootstrap, "slurm")) {
        HYDT_bsci_fns.query_proxy_id = HYDT_bscd_slurm_query_proxy_id;
        HYDT_bsci_fns.query_node_list = HYDT_bscd_slurm_query_node_list;
    }

    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_external_query_env_inherit;

    HYDU_FUNC_EXIT();

    return status;
}

HYD_status HYDT_bsci_ssh_init(void)
{
    return external_init();
}

HYD_status HYDT_bsci_rsh_init(void)
{
    return external_init();
}

HYD_status HYDT_bsci_fork_init(void)
{
    return external_init();
}

HYD_status HYDT_bsci_slurm_init(void)
{
    return external_init();
}
