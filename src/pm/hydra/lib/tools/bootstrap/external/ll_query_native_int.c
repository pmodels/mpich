/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bscd_ll_query_native_int(int *ret)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *ret = 1;

    if (!HYDTI_bscd_env_is_avail("LOADL_HOSTFILE"))
        *ret = 0;
    if (!HYDTI_bscd_env_is_avail("MP_CHILD"))
        *ret = 0;

    HYDU_FUNC_EXIT();
    return status;
}
