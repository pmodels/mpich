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

HYD_CSI_Handle * csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_CSI_Finalize"
HYD_Status HYD_CSI_Finalize()
{
    struct HYD_CSI_Proc_params * proc_params, * p;
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

    /* Clean up the control system structures now */
    HYDU_FREE(csi_handle->wdir);
    proc_params = csi_handle->proc_params;
    while (proc_params) {
	p = proc_params->next;
	HYD_CSU_Free_env_list(proc_params->env_list);
	HYD_CSU_Free_env_list(proc_params->genv_list);
	HYD_CSU_Free_exec_list(proc_params->exec);
	HYDU_FREE(proc_params->corelist);
	for (i = 0; i < proc_params->hostlist_length; i++)
	    HYDU_FREE(proc_params->hostlist[i]);
	HYDU_FREE(proc_params->hostlist);
	proc_params = p;
    }
    csi_handle->proc_params = NULL;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
