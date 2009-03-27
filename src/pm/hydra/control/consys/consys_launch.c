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

extern HYD_Handle handle;

HYD_Status HYD_CSI_launch_procs(void)
{
    struct HYD_Partition *partition;
    int stdin_fd;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCI_launch_procs();
    HYDU_ERR_POP(status, "PM returned error while launching processes\n");

    for (partition = handle.partition_list; partition; partition = partition->next) {
        status = HYD_DMX_register_fd(1, &partition->out, HYD_STDOUT, NULL, handle.stdout_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");

        if (partition->err != -1) {
            status =
                HYD_DMX_register_fd(1, &partition->err, HYD_STDOUT, NULL, handle.stderr_cb);
            HYDU_ERR_POP(status, "demux returned error registering fd\n");
        }
    }

    if (handle.in != -1) {      /* Only process_id 0 */
        status = HYDU_sock_set_nonblock(handle.in);
        HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

        stdin_fd = 0;
        status = HYDU_sock_set_nonblock(stdin_fd);
        HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

        handle.stdin_buf_count = 0;
        handle.stdin_buf_offset = 0;

        status = HYD_DMX_register_fd(1, &stdin_fd, HYD_STDIN, NULL, handle.stdin_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
