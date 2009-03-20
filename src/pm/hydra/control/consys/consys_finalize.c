/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "csi.h"
#include "pmci.h"
#include "demux.h"

HYD_Status HYD_CSI_finalize(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCI_finalize();
    HYDU_ERR_POP(status, "error returned from PM finalize\n");

    status = HYD_DMX_finalize();
    HYDU_ERR_POP(status, "error returned from demux finalize\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
