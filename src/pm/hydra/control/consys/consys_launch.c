/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "hydra_sock.h"
#include "csi.h"
#include "pmci.h"
#include "bsci.h"
#include "demux.h"

HYD_CSI_Handle * csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_CSI_Launch_procs"
HYD_Status HYD_CSI_Launch_procs()
{
    struct HYD_CSI_Proc_params * proc_params;
    int stdin_fd, flags;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCI_Launch_procs();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("process manager returned error when launching processes\n");
	goto fn_fail;
    }

    proc_params = csi_handle->proc_params;
    while (proc_params) {
	status = HYD_DMX_Register_fd(proc_params->hostlist_length, proc_params->stdout,
				     HYD_CSI_OUT, proc_params->stdout_cb);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("demux engine returned error when registering fd\n");
	    goto fn_fail;
	}

	status = HYD_DMX_Register_fd(proc_params->hostlist_length, proc_params->stderr,
				     HYD_CSI_OUT, proc_params->stderr_cb);
	if (status != HYD_SUCCESS) {
	    HYDU_Error_printf("demux engine returned error when registering fd\n");
	    goto fn_fail;
	}

	proc_params = proc_params->next;
    }

    flags = fcntl(0, F_GETFL, 0);
    if (flags < 0) {
	status = HYD_SOCK_ERROR;
	HYDU_Error_printf("unable to do fcntl on stdin\n");
	goto fn_exit;
    }
    if (fcntl(0, F_SETFL, flags | O_NONBLOCK) < 0) {
	status = HYD_SOCK_ERROR;
	HYDU_Error_printf("unable to do fcntl on stdin\n");
	goto fn_exit;
    }

    stdin_fd = 0;
    status = HYD_DMX_Register_fd(1, &stdin_fd, HYD_CSI_OUT, csi_handle->stdin_cb);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("demux engine returned error when registering fd\n");
	goto fn_fail;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
