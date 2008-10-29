/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "mpiexec.h"
#include "lchu.h"
#include "csi.h"

HYD_CSI_Handle * csi_handle;

static void usage(void)
{
    printf("Usage: ./mpiexec [global opts] [exec1 local opts] : [exec2 local opts] : ...\n\n");

    printf("Global Options (passed to all executables):\n");
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
int main(int argc, char ** argv)
{
    HYD_LCHI_Params_t params;
    struct HYD_LCHI_Local * local, * tlocal;
    HYD_CSI_Env_t * genv_list, * env, *tenv;
    HYD_CSI_Exec_t * exec, * texec;
    struct HYD_CSI_Proc_params * proc_params, * run;
    FILE * fp;
    int procs_left, current_procs, count, index, i, exit_status;
    char node[MAX_HOSTNAME_LEN + 5]; /* Give 5 extra digits for number of processes */
    char * nodename, * procs, * hostfile;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_LCHI_Get_parameters(argc, argv, &params);
    if (status != HYD_SUCCESS) {
	usage();
	exit(-1);
    }

    if (params.debug_level >= 1)
	HYD_LCHI_Dump_params(params);

    HYDU_MALLOC(csi_handle, HYD_CSI_Handle *, sizeof(HYD_CSI_Handle), status);

    csi_handle->debug_level = params.debug_level;
    csi_handle->wdir = params.global.wdir;
    csi_handle->proc_params = NULL;

    status = HYD_LCHI_Create_env_list(params.global.prop, params.global.added_env_list,
				      params.global.prop_env_list, &genv_list);
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("unable to create env list\n");
	goto fn_fail;
    }

    /* Break host files into nodes and pass it to the control
     * system. */
    local = params.local;
    hostfile = NULL;
    while (local) {
	procs_left = local->num_procs;

	HYDU_MALLOC(proc_params, struct HYD_CSI_Proc_params *, sizeof(struct HYD_CSI_Proc_params), status);
	proc_params->hostlist_length = local->num_procs;
	HYDU_MALLOC(proc_params->hostlist, char **, proc_params->hostlist_length * sizeof(char *), status);
	HYDU_MALLOC(proc_params->corelist, int *, proc_params->hostlist_length * sizeof(int), status);

	proc_params->exec = local->exec;

	proc_params->env_list = NULL;
	proc_params->genv_list = NULL;
	proc_params->next = NULL;

	index = 0;
	/* If we got a new file, open it */
	if (local->hostfile != NULL) {
	    if (hostfile != NULL) { /* We have a previously opened file */
		fclose(fp);
	    }
	    fp = fopen(local->hostfile, "r");
	    hostfile = local->hostfile;
	}

	while (1) {
	    fscanf(fp, "%s", node);

	    if (feof(fp)) {
		/* If we are at the end of the file, and more
		 * processes need to be spawned, go back to the start
		 * of the file. */
		if (procs_left != 0)
		    fseek(fp, 0, SEEK_SET);
		else
		    break;
	    }

	    nodename = strtok(node, ":");
	    procs = strtok(NULL, ":");

	    if (procs)
		current_procs = atoi(procs);
	    else
		current_procs = 1;

	    if (procs_left > current_procs) {
		procs_left -= current_procs;
	    }
	    else {
		current_procs = procs_left;
		procs_left = 0;
	    }

	    for (i = 0; i < current_procs; i++) {
		proc_params->hostlist[index] = i ? NULL : MPIU_Strdup(nodename);
		proc_params->corelist[index] = -1; /* We don't support core affinity right now. */
		index++;
	    }

	    if (!procs_left)
		break;
	}

	if (params.global.prop != HYD_LCHI_PROPAGATE_NOTSET) {
	    /* There is a global environment setting */
	    env = genv_list;
	    while (env) {
		HYDU_MALLOC(tenv, HYD_CSI_Env_t *, sizeof(HYD_CSI_Env_t), status);
		HYD_LCHI_Env_copy(tenv, *env);
		status = HYD_LCHU_Add_env_to_list(&proc_params->env_list, tenv);
		if (status != HYD_SUCCESS) {
		    HYDU_Error_printf("launcher utils returned error adding env to list\n");
		    goto fn_fail;
		}
		env = env->next;
	    }
	}
	else {
	    status = HYD_LCHI_Create_env_list(local->prop, local->added_env_list,
					      local->prop_env_list, &proc_params->env_list);
	    if (status != HYD_SUCCESS) {
		HYDU_Error_printf("unable to create env list\n");
		goto fn_fail;
	    }

	    if (local->prop == HYD_LCHI_PROPAGATE_ALL) {
		status = HYD_LCHI_Global_env_list(&proc_params->genv_list);
		if (status != HYD_SUCCESS) {
		    HYDU_Error_printf("unable to get global env list\n");
		    goto fn_fail;
		}
	    }
	    else
		proc_params->genv_list = NULL;
	}

	proc_params->stdout_cb = HYD_LCHI_stdout_cb;
	proc_params->stderr_cb = HYD_LCHI_stderr_cb;

	if (csi_handle->proc_params == NULL) {
	    csi_handle->proc_params = proc_params;
	}
	else {
	    run = csi_handle->proc_params;
	    while (run->next)
		run = run->next;
	    run->next = proc_params;
	}

	local = local->next;
    }
    fclose(fp);

    gettimeofday(&csi_handle->start, NULL);
    if (getenv("MPIEXEC_TIMEOUT"))
	csi_handle->timeout.tv_sec = atoi(getenv("MPIEXEC_TIMEOUT"));
    else {
	csi_handle->timeout.tv_sec = -1; /* Set a negative timeout */
	csi_handle->timeout.tv_usec = 0;
    }

    csi_handle->stdin_cb = HYD_LCHI_stdin_cb;

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
    proc_params = csi_handle->proc_params;
    exit_status = 0;
    while (proc_params) {
	for (i = 0; i < proc_params->hostlist_length; i++)
	    if (proc_params->exit_status[i] < 0)
		exit_status = -1;
	proc_params = proc_params->next;
    }

    /* Free the mpiexec params */
    /* FIXME: Cannot call a CSU function here */
    HYD_CSU_Free_env_list(params.global.added_env_list);
    HYD_CSU_Free_env_list(params.global.prop_env_list);

    local = params.local;
    while (local) {
	tlocal = local->next;
	if (local->hostfile)
	    HYDU_FREE(local->hostfile);

	HYD_CSU_Free_env_list(local->added_env_list);
	HYD_CSU_Free_env_list(local->prop_env_list);
	HYDU_FREE(local);

	local = tlocal;
    }

    /* Call finalize functions for lower layers to cleanup their resources */
    status = HYD_CSI_Finalize();
    if (status != HYD_SUCCESS) {
	HYDU_Error_printf("control system returned error on finalize\n");
	goto fn_fail;
    }

fn_exit:
    HYDU_FUNC_EXIT();
    if (status != HYD_SUCCESS)
	return -1;
    else
	return exit_status;

fn_fail:
    goto fn_fail;
}
