/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "bscu.h"

HYD_status HYDT_bsci_query_env_inherit(const char *env_name, int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bsci_fns.query_env_inherit) {
        status = HYDT_bsci_fns.query_env_inherit(env_name, ret);
        HYDU_ERR_POP(status, "launcher returned error querying env propagation\n");
    }
    else {
        *ret = 1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
