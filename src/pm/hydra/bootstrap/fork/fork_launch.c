/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bscu.h"
#include "fork.h"

extern HYD_Handle handle;

HYD_Status HYD_BSCD_fork_launch_procs(void)
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
    for (partition = handle.partition_list; partition; partition = partition->next) {

        /* Setup the executable arguments */
        arg = 0;
        for (i = 0; partition->proxy_args[i]; i++)
            client_arg[arg++] = HYDU_strdup(partition->proxy_args[i]);
        client_arg[arg++] = NULL;

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's NULL. */
        status = HYDU_create_process(client_arg, NULL,
                                     (process_id == 0 ? &handle.in : NULL),
                                     &partition->out, &partition->err, &partition->pid, -1);
        HYDU_ERR_POP(status, "create process returned error\n");

        for (arg = 0; client_arg[arg]; arg++)
            HYDU_FREE(client_arg[arg]);

        /* For the remaining processes, set the stdin fd to -1 */
        if (process_id != 0)
            handle.in = -1;

        process_id++;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
