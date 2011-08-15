/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pbs.h"

HYD_status HYDT_bscd_pbs_launcher_finalize(void)
{
    int err;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

#if defined(HAVE_TM_H)
    /* FIXME: Finalizing the TM library seems to be giving some weird
     * errors, so we ignore it for the time being. */
    err = tm_finalize();
    if (err != TM_SUCCESS)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "error calling tm_finalize\n");
#endif /* HAVE_TM_H */

    if (HYDT_bscd_pbs_sys) {
        if (HYDT_bscd_pbs_sys->taskobits)
            HYDU_FREE(HYDT_bscd_pbs_sys->taskobits);
        if (HYDT_bscd_pbs_sys->events)
            HYDU_FREE(HYDT_bscd_pbs_sys->events);
        if (HYDT_bscd_pbs_sys->taskIDs)
            HYDU_FREE(HYDT_bscd_pbs_sys->taskIDs);
        HYDU_FREE(HYDT_bscd_pbs_sys);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
