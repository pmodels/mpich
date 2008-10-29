/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_sock.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "csi.h"
#include "bsci.h"
#include "bscu.h"

#define MAX_CLIENT_ARG 200
#define MAX_CLIENT_ENV 200

HYD_BSCU_Procstate_t * HYD_BSCU_Procstate;
int HYD_BSCU_Num_procs;
int HYD_BSCU_Completed_procs;
HYD_CSI_Handle * csi_handle;

/*
 * HYD_BSCI_Launch_procs: For each process, we create an executable
 * which reads like "ssh exec args" and the list of environment
 * variables. We fork a worker process that sets the environment and
 * execvp's this executable.
 */
#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCI_Launch_procs"
HYD_Status HYD_BSCI_Launch_procs()
{
    struct HYD_CSI_Proc_params * proc_params;
    char * hostname, ** client_arg, ** client_env;
    int i, arg, process_id;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(client_arg, char **, MAX_CLIENT_ARG * sizeof(char *), status);
    HYDU_MALLOC(client_env, char **, MAX_CLIENT_ENV * sizeof(char *), status);

    status = HYD_BSCU_Init_exit_status();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("bootstrap utils returned error when initializing exit status\n");
	goto fn_fail;
    }

    status = HYD_BSCU_Set_common_signals(HYD_BSCU_Signal_handler);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("signal utils returned error when trying to set signal\n");
	goto fn_fail;
    }

    proc_params = csi_handle->proc_params;
    hostname = NULL;
    process_id = 0;
    while (proc_params) {
	HYDU_MALLOC(proc_params->stdout, int *, proc_params->hostlist_length * sizeof(int), status);
	HYDU_MALLOC(proc_params->stderr, int *, proc_params->hostlist_length * sizeof(int), status);

	for (i = 0; i < proc_params->hostlist_length; i++) {
	    if (hostname == NULL || proc_params->hostlist[i] != NULL) {
		hostname = proc_params->hostlist[i];
	    }

	    HYD_BSCU_Setup_env(proc_params, client_env, process_id, status);

	    /* Setup the executable arguments */
	    arg = 0;
	    client_arg[arg++] = MPIU_Strdup("/usr/bin/ssh");
	    client_arg[arg++] = MPIU_Strdup("-q");
	    client_arg[arg++] = MPIU_Strdup(hostname);

	    HYD_BSCU_Append_env(proc_params, client_env, client_arg, arg, -1);
	    client_arg[arg++] = MPIU_Strdup("cd");
	    client_arg[arg++] = MPIU_Strdup(csi_handle->wdir);
	    client_arg[arg++] = MPIU_Strdup(";");
	    HYD_BSCU_Append_exec(proc_params, client_arg, arg, -1);

	    /* The stdin pointer will be some value for process_id 0;
	     * for everyone else, it's NULL. */
	    status = HYD_BSCU_Spawn_proc(client_arg, client_env, (process_id == 0 ? &csi_handle->stdin : NULL),
					 &proc_params->stdout[i], &proc_params->stderr[i],
					 &HYD_BSCU_Procstate[process_id].pid);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("bootstrap spawn process returned error\n");
		goto fn_fail;
	    }

	    process_id++;
	}

	proc_params = proc_params->next;
    }

fn_exit:
    for (arg = 0; client_arg[arg]; arg++)
	HYDU_FREE(client_arg[arg]);
    HYDU_FREE(client_arg);

    for (arg = 0; client_env[arg]; arg++)
	HYDU_FREE(client_env[arg]);
    HYDU_FREE(client_env);

    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_BSCI_Cleanup_procs"
HYD_Status HYD_BSCI_Cleanup_procs(void)
{
    struct HYD_CSI_Proc_params * proc_params;
    char * hostname, ** client_arg, ** client_env;
    int i, arg, process_id, current_count, pid;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(client_arg, char **, MAX_CLIENT_ARG * sizeof(char *), status);
    HYDU_MALLOC(client_env, char **, MAX_CLIENT_ENV * sizeof(char *), status);

    proc_params = csi_handle->proc_params;
    hostname = NULL;
    process_id = 0;
    while (proc_params) {
	for (i = 0; i < proc_params->hostlist_length; i++) {
	    if (hostname == NULL || proc_params->hostlist[i] != NULL) {
		hostname = proc_params->hostlist[i];
	    }

	    /* Setup the executable arguments */
	    arg = 0;
	    client_arg[arg++] = MPIU_Strdup("ssh");
	    client_arg[arg++] = MPIU_Strdup("-q");
	    client_arg[arg++] = MPIU_Strdup(hostname);

	    client_arg[arg++] = MPIU_Strdup("cd");
	    client_arg[arg++] = MPIU_Strdup(csi_handle->wdir);
	    client_arg[arg++] = MPIU_Strdup(";");

	    client_arg[arg++] = MPIU_Strdup("killall");

	    pid = HYD_BSCU_Procstate[process_id].pid;
	    process_id++;
	    if (pid == -1)
		continue;

	    HYD_BSCU_Append_exec(proc_params, client_arg, arg, 1);
	    client_env[0] = NULL;

	    status = HYD_BSCU_Spawn_proc(client_arg, client_env, NULL, NULL, NULL, NULL);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("bootstrap spawn process returned error\n");
		goto fn_fail;
	    }
	}

	proc_params = proc_params->next;
    }

fn_exit:
    for (arg = 0; client_arg[arg]; arg++)
	HYDU_FREE(client_arg[arg]);
    HYDU_FREE(client_arg);

    for (arg = 0; client_env[arg]; arg++)
	HYDU_FREE(client_env[arg]);
    HYDU_FREE(client_env);

    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
