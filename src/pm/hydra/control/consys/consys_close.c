/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "csi.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"

HYD_Handle handle;

HYD_Status HYD_CSI_Close_fd(int fd)
{
    int i;
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Deregister the FD with the demux engine and close it. */
    status = HYD_DMX_Deregister_fd(fd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine returned error when deregistering fd: %d\n", fd);
        goto fn_fail;
    }

    close(fd);

    /* Find the FD in the handle and remove it. */
    proc_params = handle.proc_params;
    while (proc_params) {
        for (i = 0; i < proc_params->user_num_procs; i++) {
            if (proc_params->out[i] == fd) {
                proc_params->out[i] = -1;
                goto fn_exit;
            }
            if (proc_params->err[i] == fd) {
                proc_params->err[i] = -1;
                goto fn_exit;
            }
        }
        proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
