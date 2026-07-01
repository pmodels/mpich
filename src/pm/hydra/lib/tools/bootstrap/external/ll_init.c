/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bsci_launcher_ll_init(void)
{
    HYDT_bsci_fns.launch_procs = HYDT_bscd_ll_launch_procs;
    HYDT_bsci_fns.query_proxy_id = HYDT_bscd_ll_query_proxy_id;
    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_ll_query_env_inherit;

    return HYD_SUCCESS;
}

HYD_status HYDT_bsci_rmk_ll_init(void)
{
    HYDT_bsci_fns.query_node_list = HYDT_bscd_ll_query_node_list;
    HYDT_bsci_fns.query_native_int = HYDT_bscd_ll_query_native_int;

    return HYD_SUCCESS;
}
