/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"

HYD_status HYD_rmki_query_node_list(int *num_cores, struct HYD_proxy **proxy_list)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_rmki_fns.query_node_list(num_cores, proxy_list);
    HYDU_ERR_POP(status, "RMK device returned error while querying node list\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
