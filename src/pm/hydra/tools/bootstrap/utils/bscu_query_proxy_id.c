/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "hydra_utils.h"
#include "bscu.h"

HYD_status HYDT_bscu_query_proxy_id(int *proxy_id)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We don't know anything about proxy IDs by default. */
    *proxy_id = -1;

    HYDU_FUNC_EXIT();

    return status;
}
