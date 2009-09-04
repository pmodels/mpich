/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "rsh.h"

/*
 * HYD_BSCD_rsh_launch_procs: For each process, we create an
 * executable which reads like "rsh exec args" and the list of
 * environment variables. We fork a worker process that sets the
 * environment and execvp's this executable.
 */
HYD_Status HYD_BSCD_rsh_launch_procs(char **global_args, const char *partition_id_str,
                                     struct HYD_Partition *partition_list)
{
    struct HYD_Partition *partition;
    char *client_arg[HYD_NUM_TMP_STRINGS];
    char *tmp[HYD_NUM_TMP_STRINGS], *path = NULL, *test_path = NULL;
    int i, arg, process_id;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /*
     * We use the following priority order for the executable path:
     *    1. User-specified
     *    2. Search in path
     *    3. Hard-coded location
     */
    if (HYD_BSCI_info.bootstrap_exec) {
        path = HYDU_strdup(HYD_BSCI_info.bootstrap_exec);
    }
    else {
        status = HYDU_find_in_path("rsh", &test_path);
        HYDU_ERR_POP(status, "error while searching for executable in user path\n");

        if (test_path) {
            tmp[0] = HYDU_strdup(test_path);
            tmp[1] = HYDU_strdup("rsh");
            tmp[2] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &path);
            HYDU_ERR_POP(status, "error joining strings\n");

            HYDU_free_strlist(tmp);
        }
        else
            path = HYDU_strdup("/usr/bin/rsh");
    }

    process_id = 0;
    FORALL_ACTIVE_PARTITIONS(partition, partition_list) {
        /* Setup the executable arguments */
        arg = 0;
        client_arg[arg++] = HYDU_strdup(path);

        /* rsh does not support any partition names other than host names */
        client_arg[arg++] = HYDU_strdup(partition->base->name);

        /* Add -- to hide hydra parameters from rsh from inetutils */
        client_arg[arg++] = HYDU_strdup("--");

        for (i = 0; global_args[i]; i++)
            client_arg[arg++] = HYDU_strdup(global_args[i]);

        if (partition_id_str) {
            client_arg[arg++] = HYDU_strdup(partition_id_str);
            client_arg[arg++] = HYDU_int_to_str(partition->base->partition_id);
        }

        client_arg[arg++] = NULL;

        if (HYD_BSCI_info.debug) {
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
