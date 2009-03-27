/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "csi.h"
#include "csiu.h"
#include "pmci.h"
#include "demux.h"

extern HYD_Handle handle;

HYD_Status HYD_CSI_wait_for_completion(void)
{
    int sockets_open;
    struct HYD_Partition *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    while (1) {
        /* Wait for some event to occur */
        status = HYD_DMX_wait_for_event(HYDU_time_left(handle.start, handle.timeout));
        HYDU_ERR_POP(status, "error waiting for event\n");

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        sockets_open = 0;
        for (partition = handle.partition_list; partition; partition = partition->next) {
            if (partition->out != -1 || partition->err != -1) {
                sockets_open++;
                break;
            }
        }

        if (sockets_open && HYDU_time_left(handle.start, handle.timeout))
            continue;

        /* Make sure all the processes have terminated. The process
         * manager control device will take care of that. */
        status = HYD_PMCI_wait_for_completion();
        HYDU_ERR_POP(status, "error waiting for completion\n");

        /* We are done */
        break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
