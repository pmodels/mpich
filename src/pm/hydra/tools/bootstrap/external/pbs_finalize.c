/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
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
    err = tm_finalize();
    HYDU_ERR_CHKANDJUMP(status, err != TM_SUCCESS, HYD_INTERNAL_ERROR,
                        "error calling tm_finalize\n");
#endif /* HAVE_TM_H */

    if (HYDT_bscd_pbs_sys) {
        if (HYDT_bscd_pbs_sys->task_id)
            HYDU_FREE(HYDT_bscd_pbs_sys->task_id);
        if (HYDT_bscd_pbs_sys->spawn_events)
            HYDU_FREE(HYDT_bscd_pbs_sys->spawn_events);
        HYDU_FREE(HYDT_bscd_pbs_sys);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
