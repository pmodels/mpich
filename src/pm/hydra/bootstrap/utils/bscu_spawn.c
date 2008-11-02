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

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Init_exit_status"
HYD_Status HYD_BSCU_Init_exit_status(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Set the exit status of all processes to 1 (> 0 means that the
     * status is not set yet). Also count the number of processes in
     * the same loop. */
    HYD_BSCU_Num_procs = 0;
    proc_params = csi_handle.proc_params;
    while (proc_params) {
	HYD_BSCU_Num_procs += proc_params->user_num_procs;
	HYDU_MALLOC(proc_params->exit_status, int *, proc_params->user_num_procs * sizeof(int), status);
	for (i = 0; i < proc_params->user_num_procs; i++)
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
    struct HYD_CSI_Proc_params *proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = csi_handle.proc_params;
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
#define FUNCNAME "HYD_BSCU_Init_io_fds"
HYD_Status HYD_BSCU_Init_io_fds(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = csi_handle.proc_params;
    while (proc_params) {
	HYDU_MALLOC(proc_params->out, int *, proc_params->user_num_procs * sizeof(int), status);
	HYDU_MALLOC(proc_params->err, int *, proc_params->user_num_procs * sizeof(int), status);
	proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Init_io_fds"
HYD_Status HYD_BSCU_Finalize_io_fds(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = csi_handle.proc_params;
    while (proc_params) {
	HYDU_FREE(proc_params->out);
	HYDU_FREE(proc_params->err);
	proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Create_process"
HYD_Status HYD_BSCU_Create_process(char **client_arg, int *in, int *out, int *err, int *pid)
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
    if (tpid == 0) {	/* Child process */
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

	if (chdir(csi_handle.wdir) < 0) {
	    if (chdir(getenv("HOME")) < 0) {
		HYDU_Error_printf("unable to set working directory to %s\n", csi_handle.wdir);
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
    else {	/* Parent process */
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


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Append_env"
HYD_Status HYD_BSCU_Append_env(HYDU_Env_t * env_list, char **client_arg, int id)
{
    int i, j;
    HYDU_Env_t *env;
    char *envstr, *tmp[HYDU_NUM_JOIN_STR], *inc;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; client_arg[i]; i++);
    env = env_list;
    while (env) {
	client_arg[i++] = MPIU_Strdup("export");

	j = 0;
	tmp[j++] = MPIU_Strdup(env->env_name);
	tmp[j++] = MPIU_Strdup("=");

	if (env->env_type == HYDU_ENV_STATIC)
	    tmp[j++] = MPIU_Strdup(env->env_value);
	else if (env->env_type == HYDU_ENV_AUTOINC) {
	    HYDU_Int_to_str(env->start_val + id, inc, status);
	    tmp[j++] = MPIU_Strdup(inc);
	    HYDU_FREE(inc);
	}

	tmp[j++] = NULL;
	HYDU_STR_ALLOC_AND_JOIN(tmp, envstr, status);
	client_arg[i++] = MPIU_Strdup(envstr);
	HYDU_FREE(envstr);
	for (j = 0; tmp[j]; j++)
	    HYDU_FREE(tmp[j]);

	client_arg[i++] = MPIU_Strdup(";");

	env = env->next;
    }
    client_arg[i++] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Append_exec"
HYD_Status HYD_BSCU_Append_exec(char **exec, char **client_arg)
{
    int i, j;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; client_arg[i]; i++);
    for (j = 0; exec[j]; j++)
	client_arg[i++] = MPIU_Strdup(exec[j]);
    client_arg[i++] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCU_Append_wdir"
HYD_Status HYD_BSCU_Append_wdir(char **client_arg)
{
    int arg;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (arg = 0; client_arg[arg]; arg++);
    client_arg[arg++] = MPIU_Strdup("cd");
    client_arg[arg++] = MPIU_Strdup(csi_handle.wdir);
    client_arg[arg++] = MPIU_Strdup(";");
    client_arg[arg++] = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
