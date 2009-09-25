/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_pbs.h"

HYD_Status HYD_RMKI_pbs_init(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_RMKI_fns.query_node_list = HYD_RMKD_pbs_query_node_list;

    HYDU_FUNC_EXIT();

    return status;
}
