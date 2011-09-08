/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "pbs.h"

#if defined(HAVE_TM_H)
struct HYDT_bscd_pbs_sys *HYDT_bscd_pbs_sys;

HYD_status HYDT_bsci_launcher_pbs_init(void)
{
    int ierr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bsci_fns.launch_procs = HYDT_bscd_pbs_launch_procs;
    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_pbs_query_env_inherit;
    HYDT_bsci_fns.wait_for_completion = HYDT_bscd_pbs_wait_for_completion;
    HYDT_bsci_fns.launcher_finalize = HYDT_bscd_pbs_launcher_finalize;

    HYDU_MALLOC(HYDT_bscd_pbs_sys, struct HYDT_bscd_pbs_sys *,
                sizeof(struct HYDT_bscd_pbs_sys), status);

    /* Initialize TM and Hydra's PBS data structure: Nothing in the
     * returned tm_root is useful except tm_root.tm_nnodes which is
     * the number of processes allocated in this PBS job. */
    ierr = tm_init(NULL, &(HYDT_bscd_pbs_sys->tm_root));
    if (ierr != TM_SUCCESS)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "tm_init() fails with TM err=%d.\n",
                            ierr);
    HYDT_bscd_pbs_sys->spawned_count = 0;
    HYDT_bscd_pbs_sys->size = 0;
    HYDT_bscd_pbs_sys->taskIDs = NULL;
    HYDT_bscd_pbs_sys->events = NULL;
    HYDT_bscd_pbs_sys->taskobits = NULL;

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
    HYDT_bsci_fns.query_jobid = HYDT_bscd_pbs_query_jobid;

    return HYD_SUCCESS;
}
