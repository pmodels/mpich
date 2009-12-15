/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "poe.h"

HYD_status HYDT_bsci_poe_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bsci_fns.launch_procs = HYDT_bscd_poe_launch_procs;
    HYDT_bsci_fns.query_proxy_id = HYDT_bscd_poe_query_proxy_id;
    HYDT_bsci_fns.query_node_list = HYDT_bscd_poe_query_node_list;

    HYDU_FUNC_EXIT();

    return status;
}
