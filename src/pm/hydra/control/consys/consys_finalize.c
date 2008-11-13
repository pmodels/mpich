/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "pmci.h"
#include "bsci.h"

HYD_Status HYD_CSI_Finalize(void)
{
    struct HYD_CSI_Proc_params *proc_params, *p;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCI_Finalize();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("process manager finalize returned an error\n");
        goto fn_fail;
    }

    status = HYD_BSCI_Finalize();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("bootstrap server finalize returned an error\n");
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
