/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "lchu.h"
#include "csi.h"

HYD_Handle handle;

static void usage(void)
{
    printf("\n");
    printf("Usage: ./mpiexec [global opts] [exec1 local opts] : [exec2 local opts] : ...\n\n");

    printf("Global Options (passed to all executables):\n");
    printf("\t--verbose                        [Verbose mode]\n");
    printf("\t--version                        [Version information]\n");
    printf("\t--enable-x/--disable-x           [Enable or disable X forwarding]\n");
    printf("\t--proxy-port                     [Port on which proxies can listen]\n");
    printf("\t--bootstrap                      [Bootstrap server to use]\n");
    printf("\t--binding                        [Process binding]");

    printf("\t-genv {name} {value}             [Environment variable name and value]\n");
    printf("\t-genvlist {env1,env2,...}        [Environment variable list to pass]\n");
    printf("\t-genvnone                        [Do not pass any environment variables]\n");
    printf("\t-genvall                         [Pass all environment variables (default)]\n");
    printf("\t-f {name}                        [File containing the host names]\n");
    printf("\t-wdir {dirname}                  [Working directory to use]\n");

    printf("\n");

    printf("Local Options (passed to individual executables):\n");
    printf("\t-n/-np {value}                   [Number of processes]\n");
    printf("\t-env {name} {value}              [Environment variable name and value]\n");
    printf("\t-envlist {env1,env2,...}         [Environment variable list to pass]\n");
    printf("\t-envnone                         [Do not pass any environment variables]\n");
    printf("\t-envall                          [Pass all environment variables (default)]\n");
    printf
        ("\t{exec_name} {args}               [Name of the executable to run and its arguments]\n");

    printf("\n");
}


int main(int argc, char **argv)
{
    struct HYD_Partition *partition;
    int exit_status = 0;
    int timeout;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_LCHI_get_parameters(argv);
    if (status == HYD_GRACEFUL_ABORT) {
        exit(0);
    }
    else if (status != HYD_SUCCESS) {
        usage();
        goto fn_fail;
    }

    /* Convert the host file to a host list */
    status = HYDU_create_host_list(handle.host_file, &handle.partition_list);
    HYDU_ERR_POP(status, "unable to create host list\n");

    /* Consolidate the environment list that we need to propagate */
    status = HYD_LCHU_create_env_list();
    HYDU_ERR_POP(status, "unable to create env list\n");

    status = HYD_LCHU_merge_exec_info_to_partition();
    HYDU_ERR_POP(status, "unable to merge exec info\n");

    if (handle.debug)
        HYD_LCHU_print_params();

    handle.stdout_cb = HYD_LCHI_stdout_cb;
    handle.stderr_cb = HYD_LCHI_stderr_cb;
    handle.stdin_cb = HYD_LCHI_stdin_cb;

    HYDU_time_set(&handle.start, NULL); /* NULL implies right now */
    if (getenv("MPIEXEC_TIMEOUT"))
        timeout = atoi(getenv("MPIEXEC_TIMEOUT"));
    else
        timeout = -1;   /* Set a negative timeout */
    HYDU_Debug("Timeout set to %d seconds (negative means infinite)\n", timeout);
    HYDU_time_set(&handle.timeout, &timeout);

    /* Launch the processes */
    status = HYD_CSI_launch_procs();
    HYDU_ERR_POP(status, "control system error launching processes\n");

    /* Wait for their completion */
    status = HYD_CSI_wait_for_completion();
    HYDU_ERR_POP(status, "control system error waiting for completion\n");

    /* Check for the exit status for all the processes */
    exit_status = 0;
    for (partition = handle.partition_list; partition; partition = partition->next)
        exit_status |= partition->exit_status;

    /* Call finalize functions for lower layers to cleanup their resources */
    status = HYD_CSI_finalize();
    HYDU_ERR_POP(status, "control system error on finalize\n");

    /* Free the mpiexec params */
    HYD_LCHU_free_params();

  fn_exit:
    HYDU_FUNC_EXIT();
    if (status == HYD_GRACEFUL_ABORT)
        return 0;
    else if (status != HYD_SUCCESS)
        return -1;
    else
        return exit_status;

  fn_fail:
    goto fn_exit;
}
