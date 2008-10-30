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

HYD_BSCU_Procstate_t * HYD_BSCU_Procstate;
HYD_CSI_Handle * csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCI_Finalize"
HYD_Status HYD_BSCI_Finalize()
{
    struct HYD_CSI_Proc_params * proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCU_Finalize_exit_status();

    proc_params = csi_handle->proc_params;
    while (proc_params) {
	HYDU_FREE(proc_params->stdout);
	HYDU_FREE(proc_params->stderr);
	proc_params = proc_params->next;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
