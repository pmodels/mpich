/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "hydra_utils.h"
#include "bscu.h"

HYD_status HYDT_bscu_query_node_list(int *num_nodes, struct HYD_proxy **proxy_list)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We don't know anything about nodes or resources. Just return
     * NULL. */
    *num_nodes = 0;
    *proxy_list = NULL;

    HYDU_FUNC_EXIT();

    return status;
}
