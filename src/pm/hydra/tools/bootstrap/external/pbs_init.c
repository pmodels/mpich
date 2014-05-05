/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

#if defined(HAVE_TM_H)
struct HYDT_bscd_pbs_sys_s *HYDT_bscd_pbs_sys;

HYD_status HYDT_bsci_launcher_pbs_init(void)
{
    int err;
    struct tm_roots tm_root;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bsci_fns.launch_procs = HYDT_bscd_pbs_launch_procs;
    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_pbs_query_env_inherit;
    HYDT_bsci_fns.wait_for_completion = HYDT_bscd_pbs_wait_for_completion;
    HYDT_bsci_fns.launcher_finalize = HYDT_bscd_pbs_launcher_finalize;

    HYDU_MALLOC(HYDT_bscd_pbs_sys, struct HYDT_bscd_pbs_sys_s *,
                sizeof(struct HYDT_bscd_pbs_sys_s), status);

    /* Initialize TM and Hydra's PBS data structure */
    err = tm_init(NULL, &tm_root);
    HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                        "tm_init() failed with TM error %d\n", err);
    HYDT_bscd_pbs_sys->spawn_count = 0;
    HYDT_bscd_pbs_sys->spawn_events = NULL;
    HYDT_bscd_pbs_sys->task_id = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
#endif /* if defined(HAVE_TM_H) */

HYD_status HYDT_bsci_rmk_pbs_init(void)
{
    HYDT_bsci_fns.query_node_list = HYDT_bscd_pbs_query_node_list;
    HYDT_bsci_fns.query_native_int = HYDT_bscd_pbs_query_native_int;

    return HYD_SUCCESS;
}
