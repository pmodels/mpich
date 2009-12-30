/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "hydra_utils.h"
#include "rmku.h"

HYD_status HYD_rmku_query_node_list(struct HYD_node **node_list)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We don't know anything about nodes or resources */
    *node_list = NULL;

    HYDU_FUNC_EXIT();
    return status;
}
