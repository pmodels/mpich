/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "cssi.h"

HYD_status HYD_cssi_query_string(char *str)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_cssi_fns.query_string(str);

    HYDU_FUNC_EXIT();

    return status;
}
