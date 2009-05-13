/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "fork.h"

extern HYD_Handle handle;

HYD_Status HYD_BSCD_fork_launch_procs(char **global_args, char *partition_id_str)
{
    struct HYD_Partition *partition;
    char *client_arg[HYD_NUM_TMP_STRINGS];
    int i, arg, process_id;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: Instead of directly reading from the HYD_Handle
     * structure, the upper layers should be able to pass what exactly
     * they want launched. Without this functionality, the proxy
     * cannot use this and will have to perfom its own launch. */
    process_id = 0;
    FORALL_ACTIVE_PARTITIONS(partition, handle.partition_list) {
        /* Setup the executable arguments */
        arg = 0;

        for (i = 0; global_args[i]; i++)
            client_arg[arg++] = HYDU_strdup(global_args[i]);

        if (partition_id_str) {
            client_arg[arg++] = HYDU_strdup(partition_id_str);
            client_arg[arg++] = HYDU_int_to_str(partition->base->partition_id);
        }

        client_arg[arg++] = NULL;

        if (HYD_BSCI_debug) {
            HYDU_Dump("Launching process: ");
            HYDU_print_strlist(client_arg);
        }

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's NULL. */
        status = HYDU_create_process(client_arg, NULL,
                                     (process_id == 0 ? &partition->base->in : NULL),
                                     &partition->base->out, &partition->base->err,
                                     &partition->base->pid, -1);
        HYDU_ERR_POP(status, "create process returned error\n");

        HYDU_free_strlist(client_arg);

        process_id++;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
