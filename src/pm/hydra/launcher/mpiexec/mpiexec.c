/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "hydra_env.h"
#include "mpiexec.h"
#include "lchu.h"
#include "csi.h"

HYD_CSI_Handle csi_handle;

static void usage(void)
{
    printf("Usage: ./mpiexec [global opts] [exec1 local opts] : [exec2 local opts] : ...\n\n");

    printf("Global Options (passed to all executables):\n");
    printf("\t-v/-vv/-vvv                      [Verbose level]\n");
    printf("\t--enable-x/--disable-x           [Enable or disable X forwarding]\n");
    printf("\t-genv {name} {value}             [Environment variable name and value]\n");
    printf("\t-genvlist {env1,env2,...}        [Environment variable list to pass]\n");
    printf("\t-genvnone                        [Do not pass any environment variables]\n");
    printf("\t-genvall                         [Pass all environment variables (default)]\n");

    printf("\n");

    printf("Local Options (passed to individual executables):\n");
    printf("\t-n {value}                       [Number of processes]\n");
    printf("\t-f {name}                        [File containing the host names]\n");
    printf("\t-env {name} {value}              [Environment variable name and value]\n");
    printf("\t-envlist {env1,env2,...}         [Environment variable list to pass]\n");
    printf("\t-envnone                         [Do not pass any environment variables]\n");
    printf("\t-envall                          [Pass all environment variables (default)]\n");
    printf("\t{exec_name} {args}               [Name of the executable to run and its arguments]\n");

    printf("\n");
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "mpiexec"
int main(int argc, char **argv)
{
    struct HYD_CSI_Proc_params *proc_params;
    int exit_status, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_LCHI_Get_parameters(argc, argv);
    if (status != HYD_SUCCESS) {
	usage();
	exit(-1);
    }

    /* Convert the host file to a host list */
    status = HYD_LCHU_Create_host_list();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to create host list\n");
	goto fn_fail;
    }

    /* Consolidate the environment list that we need to propagate */
    status = HYD_LCHU_Create_env_list();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to create host list\n");
	goto fn_fail;
    }

    proc_params = csi_handle.proc_params;
    while (proc_params) {
	proc_params->stdout_cb = HYD_LCHI_stdout_cb;
	proc_params->stderr_cb = HYD_LCHI_stderr_cb;
	proc_params = proc_params->next;
    }
    csi_handle.stdin_cb = HYD_LCHI_stdin_cb;

    gettimeofday(&csi_handle.start, NULL);
    if (getenv("MPIEXEC_TIMEOUT"))
	csi_handle.timeout.tv_sec = atoi(getenv("MPIEXEC_TIMEOUT"));
    else {
	csi_handle.timeout.tv_sec = -1;	/* Set a negative timeout */
	csi_handle.timeout.tv_usec = 0;
    }

    /* Launch the processes */
    status = HYD_CSI_Launch_procs();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("control system returned error when launching processes\n");
	goto fn_fail;
    }

    /* Wait for their completion */
    status = HYD_CSI_Wait_for_completion();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("control system returned error when waiting for process' completion\n");
	goto fn_fail;
    }

    /* Check for the exit status for all the processes */
    proc_params = csi_handle.proc_params;
    exit_status = 0;
    while (proc_params) {
	for (i = 0; i < proc_params->user_num_procs; i++)
	    if (proc_params->exit_status[i] < 0)
		exit_status = -1;
	proc_params = proc_params->next;
    }

    /* Call finalize functions for lower layers to cleanup their resources */
    status = HYD_CSI_Finalize();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("control system returned error on finalize\n");
	goto fn_fail;
    }

    /* Free the mpiexec params */
    HYDU_FREE(csi_handle.wdir);
    HYD_LCHU_Free_env_list();
    HYD_LCHU_Free_exec();
    HYD_LCHU_Free_host_list();
    HYD_LCHU_Free_proc_params();

  fn_exit:
    HYDU_FUNC_EXIT();
    if (status != HYD_SUCCESS)
	return -1;
    else
	return exit_status;

  fn_fail:
    goto fn_exit;
}
