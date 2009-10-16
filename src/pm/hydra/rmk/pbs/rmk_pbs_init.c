/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_pbs.h"

HYD_status HYD_rmki_pbs_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_rmki_fns.query_node_list = HYD_rmkd_pbs_query_node_list;

    HYDU_FUNC_EXIT();

    return status;
}
