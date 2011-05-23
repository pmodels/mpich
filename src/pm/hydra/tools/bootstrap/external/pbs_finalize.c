/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pbs.h"

HYD_status HYDT_bscd_pbs_launcher_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYDT_bscd_pbs_sys) {
        if (HYDT_bscd_pbs_sys->taskobits)
            HYDU_FREE(HYDT_bscd_pbs_sys->taskobits);
        if (HYDT_bscd_pbs_sys->events)
            HYDU_FREE(HYDT_bscd_pbs_sys->events);
        if (HYDT_bscd_pbs_sys->taskIDs)
            HYDU_FREE(HYDT_bscd_pbs_sys->taskIDs);
        HYDU_FREE(HYDT_bscd_pbs_sys);
    }

    HYDU_FUNC_EXIT();

    return status;
}
