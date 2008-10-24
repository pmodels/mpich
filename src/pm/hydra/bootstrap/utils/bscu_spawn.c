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

HYD_BSCU_Procstate_t * HYD_BSCU_Procstate;
int HYD_BSCU_Num_procs;
int HYD_BSCU_Completed_procs;
HYD_CSI_Handle * csi_handle;

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Init_exit_status"
HYD_Status HYD_BSCU_Init_exit_status(void)
{
    struct HYD_CSI_Proc_params * proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Set the exit status of all processes to 1 (> 0 means that the
     * status is not set yet). Also count the number of processes in
     * the same loop. */
    HYD_BSCU_Num_procs = 0;
    proc_params = csi_handle->proc_params;
    while (proc_params) {
	HYD_BSCU_Num_procs += proc_params->hostlist_length;
	HYDU_MALLOC(proc_params->exit_status, int *, proc_params->hostlist_length * sizeof(int), status);
	for (i = 0; i < proc_params->hostlist_length; i++)
	    proc_params->exit_status[i] = 1;
	proc_params = proc_params->next;
    }

    HYDU_MALLOC(HYD_BSCU_Procstate, HYD_BSCU_Procstate_t *,
		HYD_BSCU_Num_procs * sizeof(HYD_BSCU_Procstate_t), status);
    HYD_BSCU_Completed_procs = 0;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Finalize_exit_status"
HYD_Status HYD_BSCU_Finalize_exit_status(void)
{
    struct HYD_CSI_Proc_params * proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = csi_handle->proc_params;
    while (proc_params) {
	HYDU_FREE(proc_params->exit_status);
	proc_params = proc_params->next;
    }

    HYDU_FREE(HYD_BSCU_Procstate);

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Spawn_proc"
HYD_Status HYD_BSCU_Spawn_proc(char ** client_arg, char ** client_env,
			       int * in, int * out, int * err, int * pid)
{
    int inpipe[2], outpipe[2], errpipe[2], arg, tpid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (in != NULL) {
	if (pipe(inpipe) < 0) {
	    HYDU_Error_printf("pipe error (errno: %d)\n", errno);
	    status = HYD_SOCK_ERROR;
	    goto fn_fail;
	}
    }

    if (pipe(outpipe) < 0) {
	HYDU_Error_printf("pipe error (errno: %d)\n", errno);
	status = HYD_SOCK_ERROR;
	goto fn_fail;
    }

    if (pipe(errpipe) < 0) {
	HYDU_Error_printf("pipe error (errno: %d)\n", errno);
	status = HYD_SOCK_ERROR;
	goto fn_fail;
    }

    /* Fork off the process */
    tpid = fork();
    if (tpid == 0) { /* Child process */
	close(outpipe[0]);
	close(1);
	if (dup2(outpipe[1], 1) < 0) {
	    HYDU_Error_printf("dup2 error (errno: %d)\n", errno);
	    status = HYD_SOCK_ERROR;
	    goto fn_fail;
	}

	close(errpipe[0]);
	close(2);
	if (dup2(errpipe[1], 2) < 0) {
	    HYDU_Error_printf("dup2 error (errno: %d)\n", errno);
	    status = HYD_SOCK_ERROR;
	    goto fn_fail;
	}

	if (in != NULL) {
	    close(inpipe[1]);
	    close(0);
	    if (dup2(inpipe[0], 0) < 0) {
		HYDU_Error_printf("dup2 error (errno: %d)\n", errno);
		status = HYD_SOCK_ERROR;
		goto fn_fail;
	    }
	}

	if (chdir(csi_handle->wdir) < 0) {
	    if (chdir(getenv("HOME")) < 0) {
		HYDU_Error_printf("unable to set working directory to %s\n", csi_handle->wdir);
		status = HYD_INTERNAL_ERROR;
		goto fn_fail;
	    }
	}

	/* execvp the process */
	if (execvp(client_arg[0], client_arg) < 0) {
	    HYDU_Error_printf("execvp error\n");
	    status = HYD_INTERNAL_ERROR;
	    goto fn_fail;
	}
    }
    else { /* Parent process */
	close(outpipe[1]);
	close(errpipe[1]);
	if (in)
	    *in = inpipe[1];
	if (out)
	    *out = outpipe[0];
	if (err)
	    *err = errpipe[0];
    }

    if (pid)
	*pid = tpid;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


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
HYD_Status HYD_BSCU_Wait_for_completion()
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
