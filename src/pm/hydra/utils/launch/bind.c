/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_bind_process(int core)
#if defined PROC_BINDING
{
    plpa_api_type_t p;
    plpa_cpu_set_t cpuset;
    int ret;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((plpa_api_probe(&p) == 0) && (p == PLPA_PROBE_OK)))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa probe failed\n");

    PLPA_CPU_ZERO(&cpuset);
    PLPA_CPU_SET(core, &cpuset);
    ret = plpa_sched_setaffinity(0, 1, &cpuset);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
#else
{
    return HYD_SUCCESS;
}
#endif /* PROC_BINDING */
