/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "slurm.h"

HYD_status HYDT_bscd_slurm_launch_procs(char **global_args, const char *proxy_id_str,
                                        struct HYD_proxy *proxy_list)
{
    struct HYD_proxy *proxy;
    char *client_arg[HYD_NUM_TMP_STRINGS];
    char *tmp[HYD_NUM_TMP_STRINGS], *path = NULL, *test_path = NULL;
    int i, arg, num_nodes;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /*
     * We use the following priority order for the executable path:
     *    1. User-specified
     *    2. Search in path
     *    3. Hard-coded location
     */
    if (HYDT_bsci_info.bootstrap_exec) {
        path = HYDU_strdup(HYDT_bsci_info.bootstrap_exec);
    }
    else {
        status = HYDU_find_in_path("srun", &test_path);
        HYDU_ERR_POP(status, "error while searching for executable in user path\n");

        if (test_path) {
            tmp[0] = HYDU_strdup(test_path);
            tmp[1] = HYDU_strdup("srun");
            tmp[2] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &path);
            HYDU_ERR_POP(status, "error joining strings\n");

            HYDU_free_strlist(tmp);
        }
        else
            path = HYDU_strdup("/usr/bin/srun");
    }

    arg = 0;
    client_arg[arg++] = HYDU_strdup(path);
    client_arg[arg++] = HYDU_strdup("--nodelist");

    i = 0;
    num_nodes = 0;
    FORALL_ACTIVE_PROXIES(proxy, proxy_list) {
        tmp[i++] = HYDU_strdup(proxy->info.hostname);
        if (proxy->next && proxy->next->active)
            tmp[i++] = HYDU_strdup(",");
        num_nodes++;

        /* If we used up more than half of the array elements, merge
         * what we have so far */
        if (i > (HYD_NUM_TMP_STRINGS / 2)) {
            tmp[i++] = NULL;
            status = HYDU_str_alloc_and_join(tmp, &client_arg[arg]);
            HYDU_ERR_POP(status, "error joining strings\n");

            i = 0;
            tmp[i++] = client_arg[arg];
        }
    }
    tmp[i++] = NULL;
    status = HYDU_str_alloc_and_join(tmp, &client_arg[arg]);
    HYDU_ERR_POP(status, "error joining strings\n");

    HYDU_free_strlist(tmp);

    arg++;

    client_arg[arg++] = HYDU_strdup("-N");
    client_arg[arg++] = HYDU_int_to_str(num_nodes);

    client_arg[arg++] = HYDU_strdup("-n");
    client_arg[arg++] = HYDU_int_to_str(num_nodes);

    for (i = 0; global_args[i]; i++)
        client_arg[arg++] = HYDU_strdup(global_args[i]);

    client_arg[arg++] = NULL;

    if (HYDT_bsci_info.debug) {
        HYDU_dump(stdout, "Launching process: ");
        HYDU_print_strlist(client_arg);
    }

    status = HYDU_create_process(client_arg, NULL,
                                 &proxy_list->in,
                                 &proxy_list->out, &proxy_list->err, &proxy_list->pid, -1);
    HYDU_ERR_POP(status, "bootstrap spawn process returned error\n");

    HYDU_free_strlist(client_arg);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
