/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

struct HYD_BSCI_info HYD_BSCI_info;

HYD_Status HYD_BSCD_slurm_launch_procs(char **global_args, char *partition_id_str,
                                       struct HYD_Partition *partition_list)
{
    struct HYD_Partition *partition;
    char *client_arg[HYD_NUM_TMP_STRINGS];
    char *tmp[HYD_NUM_TMP_STRINGS], *path = NULL, *test_path = NULL;
    int i, arg, num_nodes;
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
        status = HYDU_find_in_path("srun", &test_path);
        HYDU_ERR_POP(status, "error while searching for executable in user path\n");

        if (test_path) {
            tmp[0] = test_path;
            tmp[1] = "srun";
            tmp[2] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &path);
            HYDU_ERR_POP(status, "error joining strings\n");
        }
        else
            path = HYDU_strdup("/usr/bin/srun");
    }

    arg = 0;
    client_arg[arg++] = HYDU_strdup(path);
    client_arg[arg++] = HYDU_strdup("--nodelist");

    i = 0;
    num_nodes = 0;
    FORALL_ACTIVE_PARTITIONS(partition, partition_list) {
        tmp[i++] = partition->base->name;
        if (partition->next && partition->next->base->active)
            tmp[i++] = ",";
        num_nodes++;
    }
    tmp[i++] = NULL;
    status = HYDU_str_alloc_and_join(tmp, &client_arg[arg]);
    HYDU_ERR_POP(status, "error joining strings\n");

    arg++;

    client_arg[arg++] = HYDU_strdup("-N");
    client_arg[arg++] = HYDU_int_to_str(num_nodes);

    for (i = 0; global_args[i]; i++)
        client_arg[arg++] = HYDU_strdup(global_args[i]);

    client_arg[arg++] = NULL;

    if (HYD_BSCI_info.debug) {
        HYDU_Dump("Launching process: ");
        HYDU_print_strlist(client_arg);
    }

    status = HYDU_create_process(client_arg, NULL,
                                 &partition_list->base->in,
                                 &partition_list->base->out,
                                 &partition_list->base->err,
                                 &partition_list->base->pid, -1);
    HYDU_ERR_POP(status, "bootstrap spawn process returned error\n");

    HYDU_free_strlist(client_arg);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
