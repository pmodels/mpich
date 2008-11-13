/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "hydra_sock.h"
#include "csi.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"

HYD_CSI_Handle csi_handle;

HYD_Status HYD_CSI_Launch_procs(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    int stdin_fd, flags, count;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCI_Launch_procs();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("process manager returned error when launching processes\n");
        goto fn_fail;
    }

    proc_params = csi_handle.proc_params;
    while (proc_params) {
        status = HYD_DMX_Register_fd(proc_params->user_num_procs, proc_params->out,
                                     HYD_CSI_OUT, proc_params->stdout_cb);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when registering fd\n");
            goto fn_fail;
        }

        status = HYD_DMX_Register_fd(proc_params->user_num_procs, proc_params->err,
                                     HYD_CSI_OUT, proc_params->stderr_cb);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when registering fd\n");
            goto fn_fail;
        }

        proc_params = proc_params->next;
    }

    if (csi_handle.in != -1) {  /* Only process_id 0 */
        status = HYDU_Sock_set_nonblock(csi_handle.in);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("Unable to set socket as non-blocking\n");
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }

        stdin_fd = 0;
        status = HYDU_Sock_set_nonblock(stdin_fd);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("Unable to set socket as non-blocking\n");
            status = HYD_SOCK_ERROR;
            goto fn_fail;
        }

        csi_handle.stdin_buf_count = 0;
        csi_handle.stdin_buf_offset = 0;

        status = HYD_DMX_Register_fd(1, &stdin_fd, HYD_CSI_OUT, csi_handle.stdin_cb);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("demux engine returned error when registering fd\n");
            goto fn_fail;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
