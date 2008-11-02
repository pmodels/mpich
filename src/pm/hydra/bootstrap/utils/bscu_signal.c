/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_sig.h"
#include "csi.h"
#include "bsci.h"
#include "bscu.h"

HYD_BSCU_Procstate_t *HYD_BSCU_Procstate;
int HYD_BSCU_Num_procs;
int HYD_BSCU_Completed_procs;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Set_common_signals"
HYD_Status HYD_BSCU_Set_common_signals(sighandler_t handler)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Do we need a SIGCHLD signal at all? Why can't we just
     * wait in waitpid().
     *
     * Update: Initial testing seems to indicate that that just
     * waiting in waitpid() should work fine.
     */
    status = HYDU_Set_signal(SIGCHLD, handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set SIGCHLD signal\n");
	goto fn_fail;
    }

    status = HYDU_Set_signal(SIGINT, handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set SIGINT signal\n");
	goto fn_fail;
    }

    status = HYDU_Set_signal(SIGQUIT, handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set SIGQUIT signal\n");
	goto fn_fail;
    }

    status = HYDU_Set_signal(SIGTERM, handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set SIGTERM signal\n");
	goto fn_fail;
    }

#if defined SIGSTOP
    status = HYDU_Set_signal(SIGSTOP, handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set SIGSTOP signal\n");
	goto fn_fail;
    }
#endif /* SIGSTOP */

#if defined SIGCONT
    status = HYDU_Set_signal(SIGCONT, handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set SIGCONT signal\n");
	goto fn_fail;
    }
#endif /* SIGCONT */

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Signal_handler"
void HYD_BSCU_Signal_handler(int signal)
{
    int status, pid, i;

    HYDU_FUNC_ENTER();

    if (signal == SIGCHLD) {
	pid = wait(&status);

	/* If we didn't get a PID, it means that the main thread
	 * handled it. */
	if (pid <= 0)
	    goto fn_exit;

	/* Find the pid in the procstate structure and mark it as
	 * complete. */
	for (i = 0; i < HYD_BSCU_Num_procs; i++) {
	    if (HYD_BSCU_Procstate[i].pid == pid) {
		HYD_BSCU_Procstate[i].exit_status = status;
		HYD_BSCU_Completed_procs++;
		break;
	    }
	}
    }
    else if (signal == SIGINT || signal == SIGQUIT || signal == SIGTERM
#if defined SIGSTOP
	     || signal == SIGSTOP
#endif /* SIGSTOP */
#if defined SIGCONT
	     || signal == SIGCONT
#endif /* SIGSTOP */
	) {
	/* There's nothing we can do with the return value for now. */
	HYD_BSCI_Cleanup_procs();
	exit(-1);
    }
    else {
	/* Ignore other signals for now */
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return;

  fn_fail:
    goto fn_exit;
}
