/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bscu.h"
#include "slurm.h"

HYD_Handle handle;

HYD_Status HYD_BSCD_slurm_launch_procs(void)
{
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    char *client_arg[HYD_EXEC_ARGS];
    int i, arg, process_id;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Instead of directly reading from the HYD_Handle
     * structure, the upper layers should be able to pass what exactly
     * they want launched. Without this functionality, the proxy
     * cannot use this and will have to perfom its own launch. */
    process_id = 0;
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {
            if (partition->group_rank)  /* Only rank 0 is spawned */
                continue;

            /* Setup the executable arguments */
            arg = 0;
            /* FIXME: Get the path to srun */
            client_arg[arg++] = MPIU_Strdup("srun");

            /* Allow X forwarding only if explicitly requested */
            if (handle.enablex == 1)
                client_arg[arg++] = MPIU_Strdup("-X");
            else if (handle.enablex == 0)
                client_arg[arg++] = MPIU_Strdup("-x");
            else        /* default mode is disable X */
                client_arg[arg++] = MPIU_Strdup("-x");

            /* Currently, we do not support any partition names other
             * than host names */
            client_arg[arg++] = MPIU_Strdup(partition->name);

            for (i = 0; partition->args[i]; i++)
                client_arg[arg++] = MPIU_Strdup(partition->args[i]);

            client_arg[arg++] = NULL;

            /* The stdin pointer will be some value for process_id 0;
             * for everyone else, it's NULL. */
            status = HYDU_Create_process(client_arg, (process_id == 0 ? &handle.in : NULL),
                                         &partition->out, &partition->err, &partition->pid,
                                         -1);
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
