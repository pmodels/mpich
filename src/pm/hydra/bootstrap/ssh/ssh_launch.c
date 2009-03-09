/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_sock.h"
#include "hydra_mem.h"
#include "hydra_launch.h"
#include "bsci.h"
#include "bscu.h"

HYD_Handle handle;

/*
 * HYD_BSCI_Launch_procs: For each process, we create an executable
 * which reads like "ssh exec args" and the list of environment
 * variables. We fork a worker process that sets the environment and
 * execvp's this executable.
 */
HYD_Status HYD_BSCI_Launch_procs(void)
{
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    char *client_arg[HYD_EXEC_ARGS];
    int i, arg, process_id;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCU_Set_common_signals(HYD_BSCU_Signal_handler);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set signal\n");
        goto fn_fail;
    }

    process_id = 0;
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {
            if (partition->group_rank) /* Only rank 0 is spawned */
                continue;

            /* Setup the executable arguments */
            arg = 0;
            client_arg[arg++] = MPIU_Strdup("/usr/bin/ssh");

            /* Allow X forwarding only if explicitly requested */
            if (handle.enablex == 1)
                client_arg[arg++] = MPIU_Strdup("-X");
            else if (handle.enablex == 0)
                client_arg[arg++] = MPIU_Strdup("-x");
            else        /* default mode is disable X */
                client_arg[arg++] = MPIU_Strdup("-x");

            /* ssh does not support any partition names other than host names */
            client_arg[arg++] = MPIU_Strdup(partition->name);

            for (i = 0; partition->args[i]; i++)
                client_arg[arg++] = MPIU_Strdup(partition->args[i]);
            client_arg[arg] = NULL;

            /* The stdin pointer will be some value for process_id 0;
             * for everyone else, it's NULL. */
            status = HYDU_Create_process(client_arg, (process_id == 0 ? &handle.in : NULL),
                                         &partition->out, &partition->err, &partition->pid);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("bootstrap spawn process returned error\n");
                goto fn_fail;
            }

            for (arg = 0; client_arg[arg]; arg++)
                HYDU_FREE(client_arg[arg]);

            /* For the remaining processes, set the stdin fd to -1 */
            if (process_id != 0)
                handle.in = -1;

            process_id++;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_BSCI_Cleanup_procs(void)
{
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    char *client_arg[HYD_EXEC_ARGS], *hostname, **proc_list, *execname;
    int i, arg, host_id, host_id_max;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {
            /* Setup the executable arguments */
            arg = 0;
            client_arg[arg++] = MPIU_Strdup("/usr/bin/ssh");
            client_arg[arg++] = MPIU_Strdup("-x");

            /* ssh does not support any partition names other than host names */
            client_arg[arg++] = MPIU_Strdup(partition->name);
            client_arg[arg++] = NULL;

            for (arg = 0; client_arg[arg]; arg++);
            client_arg[arg++] = MPIU_Strdup("killall");

            execname = strrchr(proc_params->exec[0], '/');
            if (!execname)
                execname = proc_params->exec[0];
            else
                execname++;

            client_arg[arg++] = MPIU_Strdup(execname);
            client_arg[arg++] = NULL;

            status = HYDU_Create_process(client_arg, NULL, NULL, NULL, NULL);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("bootstrap spawn process returned error\n");
                goto fn_fail;
            }

            for (arg = 0; client_arg[arg]; arg++)
                HYDU_FREE(client_arg[arg]);
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
