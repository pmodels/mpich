/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "external.h"

static HYD_status external_rmk_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.rmk, "slurm"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_slurm_query_node_list;

    if (!strcmp(HYDT_bsci_info.rmk, "ll"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_ll_query_node_list;

    if (!strcmp(HYDT_bsci_info.rmk, "lsf"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_lsf_query_node_list;

    if (!strcmp(HYDT_bsci_info.rmk, "sge"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_sge_query_node_list;

    if (!strcmp(HYDT_bsci_info.rmk, "pbs"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_pbs_query_node_list;

    /* for everything else, set default values */
    if (HYDT_bsci_fns.query_native_int == NULL)
        HYDT_bsci_fns.query_native_int = HYDT_bscd_external_query_native_int;

    if (HYDT_bsci_fns.query_jobid == NULL)
        HYDT_bsci_fns.query_jobid = HYDT_bscd_external_query_jobid;

    HYDU_FUNC_EXIT();

    return status;
}

static HYD_status external_launcher_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.launcher, "slurm")) {
        HYDT_bsci_fns.launch_procs = HYDT_bscd_slurm_launch_procs;
        HYDT_bsci_fns.query_proxy_id = HYDT_bscd_slurm_query_proxy_id;
    }

    if (!strcmp(HYDT_bsci_info.launcher, "ll"))
        HYDT_bsci_fns.launch_procs = HYDT_bscd_ll_launch_procs;

    if (!strcmp(HYDT_bsci_info.launcher, "pbs"))
        HYDT_bsci_fns.launch_procs = HYDT_bscd_pbs_launch_procs;

    /* for everything else, set default values */
    if (HYDT_bsci_fns.launch_procs == NULL)
        HYDT_bsci_fns.launch_procs = HYDT_bscd_external_launch_procs;

    if (HYDT_bsci_fns.launcher_finalize == NULL)
        HYDT_bsci_fns.launcher_finalize = HYDT_bscd_external_launcher_finalize;

    if (HYDT_bsci_fns.query_env_inherit == NULL)
        HYDT_bsci_fns.query_env_inherit = HYDT_bscd_external_query_env_inherit;

    HYDU_FUNC_EXIT();

    return status;
}

HYD_status HYDT_bsci_launcher_ssh_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_rsh_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_fork_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_lsf_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_sge_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_slurm_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_ll_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_launcher_manual_init(void)
{
    return external_launcher_init();
}

HYD_status HYDT_bsci_rmk_lsf_init(void)
{
    return external_rmk_init();
}

HYD_status HYDT_bsci_rmk_sge_init(void)
{
    return external_rmk_init();
}

HYD_status HYDT_bsci_rmk_slurm_init(void)
{
    return external_rmk_init();
}

HYD_status HYDT_bsci_rmk_ll_init(void)
{
    return external_rmk_init();
}

HYD_status HYDT_bsci_rmk_pbs_init(void)
{
    return external_rmk_init();
}

HYD_status HYDT_bsci_rmk_user_init(void)
{
    return external_rmk_init();
}
