/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "csi.h"
#include "pmci.h"
#include "demux.h"

HYD_Status HYD_CSI_Finalize(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCI_Finalize();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("process manager finalize returned an error\n");
        goto fn_fail;
    }

    status = HYD_DMX_Finalize();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine finalize returned an error\n");
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
