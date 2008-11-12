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

HYD_CSI_Handle csi_handle;

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
HYD_Status HYD_BSCI_Launch_procs(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    char *client_arg[HYD_CSI_EXEC_ARGS], *hostname = NULL, **proc_list = NULL;
    int i, arg, process_id, host_id, host_id_max;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

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

    status = HYD_BSCU_Init_io_fds();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("bootstrap utils returned error when initializing io fds\n");
	goto fn_fail;
    }

    proc_params = csi_handle.proc_params;
    process_id = 0;
    while (proc_params) {
	if (proc_params->host_file != NULL) {	/* We got a new host file */
	    host_id = 0;
	    host_id_max = proc_params->total_num_procs;
	    proc_list = proc_params->total_proc_list;
	}

	for (i = 0; i < proc_params->user_num_procs; i++) {
	    /* Setup the executable arguments */
	    arg = 0;
	    client_arg[arg++] = MPIU_Strdup("/usr/bin/ssh");

	    /* Allow X forwarding only if explicitly requested */
	    if (csi_handle.enablex == 1)
		client_arg[arg++] = MPIU_Strdup("-Xq");
	    else if (csi_handle.enablex == 0)
		client_arg[arg++] = MPIU_Strdup("-xq");
	    else	/* default mode is disable X */
		client_arg[arg++] = MPIU_Strdup("-xq");

	    if (host_id == host_id_max)
		host_id = 0;
	    hostname = proc_list[host_id];
	    host_id++;

	    client_arg[arg++] = MPIU_Strdup(hostname);
	    client_arg[arg++] = NULL;

	    HYD_BSCU_Append_env(csi_handle.system_env, client_arg, process_id);
	    HYD_BSCU_Append_env(proc_params->prop_env, client_arg, process_id);
	    HYD_BSCU_Append_wdir(client_arg);
	    HYD_BSCU_Append_exec(proc_params->exec, client_arg);

	    /* The stdin pointer will be some value for process_id 0;
	     * for everyone else, it's NULL. */
	    status = HYD_BSCU_Create_process(client_arg, (process_id == 0 ? &csi_handle.in : NULL),
					     &proc_params->out[i], &proc_params->err[i],
					     &proc_params->pid[i]);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("bootstrap spawn process returned error\n");
		goto fn_fail;
	    }

	    for (arg = 0; client_arg[arg]; arg++)
		HYDU_FREE(client_arg[arg]);

	    /* For the remaining processes, set the stdin fd to -1 */
	    if (process_id != 0)
		csi_handle.in = -1;

	    process_id++;
	}

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
#define FUNCNAME "HYD_BSCI_Cleanup_procs"
HYD_Status HYD_BSCI_Cleanup_procs(void)
{
    struct HYD_CSI_Proc_params *proc_params;
    char *client_arg[HYD_CSI_EXEC_ARGS], *hostname, **proc_list;
    int i, arg, host_id, host_id_max;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = csi_handle.proc_params;
    while (proc_params) {
	for (i = 0; i < proc_params->user_num_procs; i++) {
	    /* Setup the executable arguments */
	    arg = 0;
	    client_arg[arg++] = MPIU_Strdup("/usr/bin/ssh");
	    client_arg[arg++] = MPIU_Strdup("-xq");

	    if (proc_params->host_file != NULL) {	/* We got a new host file */
		host_id = 0;
		host_id_max = proc_params->total_num_procs;
		proc_list = proc_params->total_proc_list;
	    }
	    else if (host_id == host_id_max) {
		host_id = 0;
	    }
	    hostname = proc_list[host_id];
	    host_id++;

	    client_arg[arg++] = MPIU_Strdup(hostname);
	    client_arg[arg++] = NULL;

	    HYD_BSCU_Append_wdir(client_arg);

            for (arg = 0; client_arg[arg]; arg++);
	    client_arg[arg++] = MPIU_Strdup("killall");
	    client_arg[arg++] = NULL;

	    /* FIXME: We need to free the remaining args */
	    proc_params->exec[1] = 0;	/* We only care about the executable name */
	    HYD_BSCU_Append_exec(proc_params->exec, client_arg);

	    status = HYD_BSCU_Create_process(client_arg, NULL, NULL, NULL, NULL);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("bootstrap spawn process returned error\n");
		goto fn_fail;
	    }

	    for (arg = 0; client_arg[arg]; arg++)
		HYDU_FREE(client_arg[arg]);
	}

	proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
