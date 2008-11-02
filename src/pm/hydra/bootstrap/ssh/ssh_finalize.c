/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "csi.h"
#include "bsci.h"
#include "bscu.h"

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCI_Finalize"
HYD_Status HYD_BSCI_Finalize(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCU_Finalize_exit_status();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to finalize exit status\n");
	goto fn_fail;
    }

    status = HYD_BSCU_Finalize_io_fds();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to finalize I/O fds\n");
	goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
