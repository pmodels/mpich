/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "bsci.h"
#include "bscu.h"

HYD_Handle handle;

HYD_Status HYD_BSCU_Finalize_exit_status(void)
{
    struct HYD_Proc_params *proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = handle.proc_params;
    while (proc_params) {
        HYDU_FREE(proc_params->pid);
        HYDU_FREE(proc_params->exit_status);
        HYDU_FREE(proc_params->exit_status_valid);
        proc_params = proc_params->next;
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_BSCU_Finalize_io_fds(void)
{
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = handle.proc_params;
    while (proc_params) {
        HYDU_FREE(proc_params->out);
        HYDU_FREE(proc_params->err);
        proc_params = proc_params->next;
    }

    HYDU_FUNC_EXIT();
    return status;
}
