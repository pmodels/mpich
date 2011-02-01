/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_query_usize(int *size)
{
    struct HYD_node *node_list, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.query_usize)
        return HYDT_bsci_fns.query_usize(size);

    status = HYDT_bsci_query_node_list(&node_list);
    HYDU_ERR_POP(status, "unable to query node list\n");

    if (node_list) {
        *size = 0;
        for (tmp = node_list; tmp; tmp = tmp->next)
            *size += tmp->core_count;
    }
    else {
        *size = -1;
    }

    HYDU_free_node_list(node_list);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
