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
    int arg, i, count;
    struct HYD_Partition_list *partition, *run;
    HYD_Env_t *found, env;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_Proxy_params.global_env = NULL;
    HYD_Proxy_params.env_list = NULL;
    HYD_Proxy_params.partition = NULL;

    status = HYDU_Env_global_list(&HYD_Proxy_params.global_env);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to get the global env list\n");
        goto fn_fail;
    }

    while (--argc && ++argv) {

        /* Process count */
        if (!strcmp(*argv, "--proc-count")) {
            argv++;
            HYD_Proxy_params.proc_count = atoi(*argv);
            continue;
        }

        /* Proxy port */
        if (!strcmp(*argv, "--proxy-port")) {
            argv++;
            HYD_Proxy_params.proxy_port = atoi(*argv);
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

        /* Environment information is passed as a list of names; we
         * need to find the values from our environment. */
        if (!strcmp(*argv, "--environment")) {
            argv++;
            count = atoi(*argv);
            for (i = 0; i < count; i++) {
                argv++;

                /* First find the environment variable */
                env.env_name = MPIU_Strdup(*argv);
                found = HYDU_Env_found_in_list(HYD_Proxy_params.global_env, env);
                if (!found) {
                    HYDU_Error_printf("Unable to find requested env: %s\n", env.env_name);
                    status = HYD_INTERNAL_ERROR;
                    goto fn_fail;
                }

                /* Now add it to the env_list */
                status = HYDU_Env_add_to_list(&HYD_Proxy_params.env_list, *found);
                if (status != HYD_SUCCESS) {
                    HYDU_Error_printf("Unable to add env to list\n");
                    goto fn_fail;
                }
            }
            continue;
        }

        /* Fall through case is application parameters. Load
         * everything into the args variable. */
        for (arg = 0; *argv;) {
            HYD_Proxy_params.args[arg++] = MPIU_Strdup(*argv);
            ++argv;
            --argc;
        }
        HYD_Proxy_params.args[arg++] = NULL;
        break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
