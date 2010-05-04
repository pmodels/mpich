/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "rmki.h"
#include "rmk_lsf.h"

HYD_status HYDT_rmki_lsf_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_rmki_fns.query_node_list = HYDT_rmkd_lsf_query_node_list;

    HYDU_FUNC_EXIT();

    return status;
}
