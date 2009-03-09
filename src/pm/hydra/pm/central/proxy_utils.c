/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "proxy.h"

struct HYD_Proxy_params HYD_Proxy_params;

HYD_Status HYD_Proxy_get_params(int t_argc, char **t_argv)
{
    int argc = t_argc;
    char **argv = t_argv;
    int app_params = 0, arg;
    struct HYD_Partition_list *partition, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_Proxy_params.global_env = NULL;
    HYD_Proxy_params.partition = NULL;

    status = HYDU_Env_global_list(&HYD_Proxy_params.global_env);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to get the global env list\n");
        goto fn_fail;
    }

    app_params = 0;
    while (--argc && ++argv) {

        /* Process count */
        if (!strcmp(*argv, "--proc-count")) {
            argv++;
            HYD_Proxy_params.proc_count = atoi(*argv);
            continue;
        }

        /* PMI_ID: This is the PMI_ID for the first process;
         * everything else is incremented from here. */
        if (!strcmp(*argv, "--pmi-id")) {
            argv++;
            HYD_Proxy_params.pmi_id = atoi(*argv);
            continue;
        }

        /* Partition information is passed as two parameters; name
         * followed by proc count. Multiple partitions are specified
         * as multiple parameters. */
        if (!strcmp(*argv, "--partition")) {
            argv++;
            HYDU_Allocate_Partition(&partition);
            partition->name = MPIU_Strdup(*argv);
            argv++;
            partition->proc_count = atoi(*argv);

            if (!HYD_Proxy_params.partition)
                HYD_Proxy_params.partition = partition;
            else {
                for (run = HYD_Proxy_params.partition; run->next; run = run->next);
                run->next = partition;
            }
            continue;
        }

        /* Fall through case is application parameters. Load
         * everything into the args variable. */
        for (arg = 0; argv;)
            HYD_Proxy_params.args[arg++] = MPIU_Strdup(argv++);
        HYD_Proxy_params.args[arg++] = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
