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
#include "demux.h"

extern HYD_Handle handle;

static void usage(void)
{
    printf("\n");
    printf("Usage: ./mpiexec [global opts] [exec1 local opts] : [exec2 local opts] : ...\n\n");

    printf("Global Options (passed to all executables):\n");
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
    printf("\t{exec_name} {args}               [Executable name to run and arguments]\n");

    printf("\n");

    printf("Hydra specific options (treated as global):\n");
    printf("\t--verbose                        [Verbose mode]\n");
    printf("\t--version                        [Version information]\n");
    printf("\t--enable-x/--disable-x           [Enable or disable X forwarding]\n");
    printf("\t--proxy-port                     [Port on which proxies can listen]\n");
    printf("\t--bootstrap                      [Bootstrap server to use]\n");
    printf("\t--css                            [Communication sub-system to use]\n");
    printf("\t--binding                        [Process binding]\n");
    printf("\t--boot-proxies                   [Boot proxies to run in persistent mode]\n");
    printf("\t--boot-foreground-proxies        [Boot foreground proxies (persistent mode)]\n");
    printf("\t--shutdown-proxies               [Shutdown persistent mode proxies]\n");
    printf("\t--use-persistent                 [Use persistent mode proxies to launch]\n");
    printf("\t--ranks-per-proc                 [Assign so many ranks to each process]\n");
    printf("\t--bootstrap-exec                 [Executable to use to bootstrap processes]\n");
}


int main(int argc, char **argv)
{
    struct HYD_Partition *partition;
    int exit_status = 0;
    int timeout, stdin_fd;
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

    /* During shutdown, no processes are launched, so there is nothing
     * to wait for. If the launch command didn't return an error, we
     * are OK; just return a success. */
    /* FIXME: We are assuming a working model for the process manager
     * here. We need to get how many processes the PM has launched
     * instead of assuming this. For example, it is possible to have a
     * PM implementation that launches separate "new" proxies on a
     * different port and kills the original proxies using them. */
    if (handle.launch_mode == HYD_LAUNCH_SHUTDOWN) {
        /* Call finalize functions for lower layers to cleanup their resources */
        status = HYD_CSI_finalize();
        HYDU_ERR_POP(status, "control system error on finalize\n");

        /* Free the mpiexec params */
        HYD_LCHU_free_params();

        exit_status = 0;
        goto fn_exit;
    }

    /* Setup stdout/stderr/stdin handlers */
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next) {
        status = HYD_DMX_register_fd(1, &partition->out, HYD_STDOUT, NULL, HYD_LCHI_stdout_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");

        status = HYD_DMX_register_fd(1, &partition->err, HYD_STDOUT, NULL, HYD_LCHI_stderr_cb);
        HYDU_ERR_POP(status, "demux returned error registering fd\n");
    }

    status = HYDU_sock_set_nonblock(handle.in);
    HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

    stdin_fd = 0;
    status = HYDU_sock_set_nonblock(stdin_fd);
    HYDU_ERR_POP(status, "unable to set socket as non-blocking\n");

    handle.stdin_buf_count = 0;
    handle.stdin_buf_offset = 0;

    status = HYD_DMX_register_fd(1, &stdin_fd, HYD_STDIN, NULL, HYD_LCHI_stdin_cb);
    HYDU_ERR_POP(status, "demux returned error registering fd\n");


    /* Wait for their completion */
    status = HYD_CSI_wait_for_completion();
    HYDU_ERR_POP(status, "control system error waiting for completion\n");

    /* Check for the exit status for all the processes */
    exit_status = 0;
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next)
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
