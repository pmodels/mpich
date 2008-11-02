/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "bsci.h"
#include "bscu.h"

HYD_BSCU_Procstate_t *HYD_BSCU_Procstate;
int HYD_BSCU_Num_procs;
int HYD_BSCU_Completed_procs;
HYD_CSI_Handle csi_handle;

/*
 * HYD_BSCU_Wait_for_completion: We first wait for communication
 * events from the available processes till all connections have
 * closed. In the meanwhile, the SIGCHLD handler keeps track of all
 * the terminated processes.
 */
#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Wait_for_completion"
HYD_Status HYD_BSCU_Wait_for_completion(void)
{
    int pid, ret_status, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
	pid = waitpid(-1, &ret_status, WNOHANG);
	if (pid > 0) {
	    /* Find the pid in the procstate structure and mark it as
	     * complete. */
	    for (i = 0; i < HYD_BSCU_Num_procs; i++) {
		if (HYD_BSCU_Procstate[i].pid == pid) {
		    HYD_BSCU_Procstate[i].exit_status = ret_status;
		    HYD_BSCU_Completed_procs++;
		    break;
		}
	    }
	}
	if (HYD_CSU_Time_left() == 0) {
	    status = HYD_BSCI_Cleanup_procs();
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("bootstrap process cleanup failed\n");
		goto fn_fail;
	    }
	    break;
	}
    } while (pid > 0);

    /* We need a better way of doing this; maybe semaphores? */
    while (1) {
	if (HYD_CSU_Time_left() == 0) {
	    status = HYD_BSCI_Cleanup_procs();
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("bootstrap process cleanup failed\n");
		goto fn_fail;
	    }
	    break;
	}
	if (HYD_BSCU_Completed_procs == HYD_BSCU_Num_procs)
	    break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
