/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "bscu.h"
#include "fork.h"

HYD_status HYDT_bscd_fork_launch_procs(char **global_args, const char *proxy_id_str,
                                       struct HYD_proxy *proxy_list)
{
    struct HYD_proxy *proxy;
    char *client_arg[HYD_NUM_TMP_STRINGS];
    int i, arg, process_id;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    process_id = 0;
    FORALL_ACTIVE_PROXIES(proxy, proxy_list) {
        /* Setup the executable arguments */
        arg = 0;

        for (i = 0; global_args[i]; i++)
            client_arg[arg++] = HYDU_strdup(global_args[i]);

        if (proxy_id_str) {
            client_arg[arg++] = HYDU_strdup(proxy_id_str);
            client_arg[arg++] = HYDU_int_to_str(proxy->proxy_id);
        }

        client_arg[arg++] = NULL;

        if (HYDT_bsci_info.debug) {
            HYDU_dump(stdout, "Launching process: ");
            HYDU_print_strlist(client_arg);
        }

        /* The stdin pointer will be some value for process_id 0; for
         * everyone else, it's NULL. */
        status = HYDU_create_process(client_arg, NULL,
                                     (process_id == 0 ? &proxy->in : NULL),
                                     &proxy->out, &proxy->err, &proxy->pid, -1);
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
