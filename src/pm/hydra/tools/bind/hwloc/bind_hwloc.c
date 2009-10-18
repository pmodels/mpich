/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bind.h"
#include "bind_hwloc.h"

struct HYDT_bind_info HYDT_bind_info;

HYD_status HYDT_bind_hwloc_init(HYDT_bind_support_level_t * support_level)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDT_bind_hwloc_process(int core)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
