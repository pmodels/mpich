/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "cssi.h"
#include "none.h"

struct HYD_CSSI_fns HYD_CSSI_fns;

HYD_Status HYD_CSSI_none_init(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_CSSI_fns.query_string = HYD_CSSD_none_query_string;
    HYD_CSSI_fns.finalize = HYD_CSSD_none_finalize;

    HYDU_FUNC_EXIT();

    return status;
}
