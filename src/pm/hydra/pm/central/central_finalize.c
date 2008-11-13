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
#include "central.h"

int HYD_PMCD_Central_listenfd;

HYD_Status HYD_PMCI_Finalize(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Deregister the listen socket from the demux engine and close
     * it. */
    status = HYD_DMX_Deregister_fd(HYD_PMCD_Central_listenfd);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine returned error when deregistering fd\n");
        goto fn_fail;
    }

    close(HYD_PMCD_Central_listenfd);
    HYD_PMCD_Central_listenfd = -1;

    status = HYD_PMCU_Finalize();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to finalize process manager utils\n");
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
