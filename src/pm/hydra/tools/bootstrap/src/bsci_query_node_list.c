/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_query_node_list(struct HYD_node **node_list)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.query_node_list) {
        status = HYDT_bsci_fns.query_node_list(node_list);
        HYDU_ERR_POP(status, "RMK returned error while querying node list\n");
    }
    else {
        /* We don't know anything about nodes or resources */
        *node_list = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
