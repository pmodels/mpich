/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_query_node_list(int *num_nodes, struct HYD_proxy **proxy_list)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_fns.query_node_list(num_nodes, proxy_list);
    HYDU_ERR_POP(status, "bootstrap device returned error while querying node list\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
