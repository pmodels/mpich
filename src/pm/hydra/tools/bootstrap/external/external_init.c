/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "external.h"

static HYD_status external_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!strcmp(HYDT_bsci_info.bootstrap, "ssh"))
        HYDT_bsci_fns.launch_procs = HYDT_bscd_external_launch_procs;

    if (!strcmp(HYDT_bsci_info.bootstrap, "lsf"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_lsf_query_node_list;

    if (!strcmp(HYDT_bsci_info.bootstrap, "sge"))
        HYDT_bsci_fns.query_node_list = HYDT_bscd_sge_query_node_list;

    /* rsh, fork use the default values */

    if (!strcmp(HYDT_bsci_info.bootstrap, "slurm")) {
        HYDT_bsci_fns.launch_procs = HYDT_bscd_slurm_launch_procs;
        HYDT_bsci_fns.query_proxy_id = HYDT_bscd_slurm_query_proxy_id;
        HYDT_bsci_fns.query_node_list = HYDT_bscd_slurm_query_node_list;
    }

    if (!strcmp(HYDT_bsci_info.bootstrap, "ll")) {
        HYDT_bsci_fns.launch_procs = HYDT_bscd_ll_launch_procs;
        HYDT_bsci_fns.query_proxy_id = HYDT_bscd_ll_query_proxy_id;
        HYDT_bsci_fns.query_node_list = HYDT_bscd_ll_query_node_list;
    }

    /* set default values */
    if (HYDT_bsci_fns.launch_procs == NULL)
        HYDT_bsci_fns.launch_procs = HYDT_bscd_external_launch_procs;

    if (HYDT_bsci_fns.finalize == NULL)
        HYDT_bsci_fns.finalize = HYDT_bscd_external_finalize;

    if (HYDT_bsci_fns.query_env_inherit == NULL)
        HYDT_bsci_fns.query_env_inherit = HYDT_bscd_external_query_env_inherit;

    if (HYDT_bsci_fns.query_native_int == NULL)
        HYDT_bsci_fns.query_native_int = HYDT_bscd_external_query_native_int;

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

HYD_status HYDT_bsci_lsf_init(void)
{
    return external_init();
}

HYD_status HYDT_bsci_sge_init(void)
{
    return external_init();
}

HYD_status HYDT_bsci_slurm_init(void)
{
    return external_init();
}

HYD_status HYDT_bsci_ll_init(void)
{
    return external_init();
}
