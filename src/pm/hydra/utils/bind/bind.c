/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_bind_process(int core)
{
    plpa_api_type_t p;
    plpa_cpu_set_t cpuset;
    int ret, supported;
    int num_procs, max_proc_id;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (!((plpa_api_probe(&p) == 0) && (p == PLPA_PROBE_OK))) {
        /* If this failed, we just return without binding */
        goto fn_exit;
    }

    /* We need topology information too */
    ret = plpa_have_topology_information(&supported);
    if ((ret == 0) && (supported == 1)) {
        /* Find the maximum number of processing elements */
        ret = plpa_get_processor_data(PLPA_COUNT_ALL, &num_procs, &max_proc_id);
        if (ret) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "plpa get processor data failed\n");
        }
    }
    else {
        /* If this failed, we just return without binding */
        goto fn_exit;
    }

    PLPA_CPU_ZERO(&cpuset);
    PLPA_CPU_SET(core % num_procs, &cpuset);
    ret = plpa_sched_setaffinity(0, 1, &cpuset);
    if (ret)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "plpa setaffinity failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
