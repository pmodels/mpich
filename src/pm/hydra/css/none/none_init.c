/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "cssi.h"
#include "none.h"

HYD_status HYD_cssi_none_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_cssi_fns.query_string = HYD_cssd_none_query_string;
    HYD_cssi_fns.finalize = HYD_cssd_none_finalize;

    HYDU_FUNC_EXIT();

    return status;
}
