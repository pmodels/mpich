/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "csi.h"
#include "pmci.h"
#include "demux.h"

HYD_Handle handle;

HYD_Status HYD_CSI_Close_fd(int fd)
{
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Deregister the FD with the demux engine and close it. */
    status = HYD_DMX_Deregister_fd(fd);
    HYDU_ERR_SETANDJUMP1(status, status, "error deregistering fd %d\n", fd);
    close(fd);

    /* Find the FD in the handle and remove it. */
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {
            if (partition->out == fd) {
                partition->out = -1;
                goto fn_exit;
            }
            if (partition->err == fd) {
                partition->err = -1;
                goto fn_exit;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
