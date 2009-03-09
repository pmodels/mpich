/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "csi.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"

HYD_Handle handle;

HYD_Status HYD_CSI_Wait_for_completion(void)
{
    int sockets_open, i;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    while (1) {
        /* Wait for some event to occur */
        status = HYD_DMX_Wait_for_event();
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf
                ("demux engine returned error when waiting for event\n");
            goto fn_fail;
        }

        /* Check to see if there's any open read socket left; if there
         * are, we will just wait for more events. */
        sockets_open = 0;
        for (proc_params = handle.proc_params; proc_params;
             proc_params = proc_params->next) {
            for (partition = proc_params->partition; partition;
                 partition = partition->next) {
                if (partition->out != -1 || partition->err != -1) {
                    sockets_open++;
                    break;
                }
            }
            if (sockets_open)
                break;
        }

        if (sockets_open && HYD_CSU_Time_left())
            continue;

        /* Make sure all the processes have terminated. The bootstrap
         * control device will take care of that. */
        status = HYD_BSCI_Wait_for_completion();
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf
                ("bootstrap server returned error when waiting for completion\n");
            goto fn_fail;
        }

        /* We are done */
        break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
