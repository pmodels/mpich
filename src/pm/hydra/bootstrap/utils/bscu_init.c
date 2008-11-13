/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "bsci.h"
#include "bscu.h"

HYD_CSI_Handle csi_handle;

HYD_Status HYD_BSCU_Init_exit_status(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Set the exit status of all processes to 1 (> 0 means that the
     * status is not set yet). Also count the number of processes in
     * the same loop. */
    proc_params = csi_handle.proc_params;
    while (proc_params) {
        HYDU_MALLOC(proc_params->pid, int *, proc_params->user_num_procs * sizeof(int), status);
        HYDU_MALLOC(proc_params->exit_status, int *, proc_params->user_num_procs * sizeof(int), status);
        HYDU_MALLOC(proc_params->exit_status_valid, int *, proc_params->user_num_procs * sizeof(int),
                    status);
        for (i = 0; i < proc_params->user_num_procs; i++)
            proc_params->exit_status_valid[i] = 0;
        proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_BSCU_Init_io_fds(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = csi_handle.proc_params;
    while (proc_params) {
        HYDU_MALLOC(proc_params->out, int *, proc_params->user_num_procs * sizeof(int), status);
        HYDU_MALLOC(proc_params->err, int *, proc_params->user_num_procs * sizeof(int), status);
        proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
