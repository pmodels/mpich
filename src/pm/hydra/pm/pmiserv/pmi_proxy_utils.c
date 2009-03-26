/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmi_proxy.h"
#include "hydra_utils.h"

struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;

HYD_Status HYD_PMCD_pmi_proxy_get_params(int t_argc, char **t_argv)
{
    char **argv = t_argv, *str;
    int arg, i, count;
    HYD_Env_t *env;
    struct HYD_Partition_exec *exec = NULL;
    struct HYD_Partition_segment *segment = NULL;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_PMCD_pmi_proxy_params.exec_list = NULL;
    HYD_PMCD_pmi_proxy_params.segment_list = NULL;
    HYD_PMCD_pmi_proxy_params.global_env = NULL;

    while (*argv) {
        ++argv;
        if (*argv == NULL)
            break;

        /* Proxy port */
        if (!strcmp(*argv, "--proxy-port")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.proxy_port = atoi(*argv);
            continue;
        }

        /* Working directory */
        if (!strcmp(*argv, "--wdir")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.wdir = HYDU_strdup(*argv);
            continue;
        }

        /* Working directory */
        if (!strcmp(*argv, "--binding")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.binding = atoi(*argv);
            argv++;
            if (!strcmp(*argv, "HYDRA_NO_USER_MAP"))
                HYD_PMCD_pmi_proxy_params.user_bind_map = NULL;
            else
                HYD_PMCD_pmi_proxy_params.user_bind_map = HYDU_strdup(*argv);
            continue;
        }

        /* Global env */
        if (!strcmp(*argv, "--global-env")) {
            argv++;
            count = atoi(*argv);
            for (i = 0; i < count; i++) {
                argv++;
                str = *argv;

                /* Some bootstrap servers remove the quotes that we
                 * added, while some others do not. For the cases
                 * where they are not removed, we do it ourselves. */
                if (*str == '\'') {
                    str++;
                    str[strlen(str) - 1] = 0;
                }
                env = HYDU_str_to_env(str);
                HYDU_append_env_to_list(*env, &HYD_PMCD_pmi_proxy_params.global_env);
                HYDU_FREE(env);
            }
            continue;
        }

        /* One-pass Count */
        if (!strcmp(*argv, "--one-pass-count")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.one_pass_count = atoi(*argv);
            continue;
        }

        /* New segment */
        if (!strcmp(*argv, "--segment")) {
            if (HYD_PMCD_pmi_proxy_params.segment_list == NULL) {
                status = HYDU_alloc_partition_segment(&HYD_PMCD_pmi_proxy_params.segment_list);
                HYDU_ERR_POP(status, "unable to allocate partition segment\n");
            }
            else {
                for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment->next;
                     segment = segment->next);
                status = HYDU_alloc_partition_segment(&segment->next);
                HYDU_ERR_POP(status, "unable to allocate partition segment\n");
            }
            continue;
        }

        /* Process count */
        if (!strcmp(*argv, "--segment-proc-count")) {
            argv++;
            for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment->next;
                 segment = segment->next);
            segment->proc_count = atoi(*argv);
            continue;
        }

        /* Process count */
        if (!strcmp(*argv, "--segment-start-pid")) {
            argv++;
            for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment->next;
                 segment = segment->next);
            segment->start_pid = atoi(*argv);
            continue;
        }

        /* New executable */
        if (!strcmp(*argv, "--exec")) {
            if (HYD_PMCD_pmi_proxy_params.exec_list == NULL) {
                status = HYDU_alloc_partition_exec(&HYD_PMCD_pmi_proxy_params.exec_list);
                HYDU_ERR_POP(status, "unable to allocate partition exec\n");
            }
            else {
                for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next;
                     exec = exec->next);
                status = HYDU_alloc_partition_exec(&exec->next);
                HYDU_ERR_POP(status, "unable to allocate partition exec\n");
            }
            continue;
        }

        /* Process count */
        if (!strcmp(*argv, "--exec-proc-count")) {
            argv++;
            for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next; exec = exec->next);
            exec->proc_count = atoi(*argv);
            continue;
        }

        /* Local env */
        if (!strcmp(*argv, "--exec-local-env")) {
            argv++;
            count = atoi(*argv);
            for (i = 0; i < count; i++) {
                argv++;
                str = *argv;

                /* Some bootstrap servers remove the quotes that we
                 * added, while some others do not. For the cases
                 * where they are not removed, we do it ourselves. */
                if (*str == '\'') {
                    str++;
                    str[strlen(str) - 1] = 0;
                }
                env = HYDU_str_to_env(str);
                HYDU_append_env_to_list(*env, &exec->prop_env);
                HYDU_FREE(env);
            }
            continue;
        }

        /* Fall through case is application parameters. Load
         * everything into the args variable. */
        for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next; exec = exec->next);
        for (arg = 0; *argv && strcmp(*argv, "--exec");) {
            exec->exec[arg++] = HYDU_strdup(*argv);
            ++argv;
        }
        exec->exec[arg++] = NULL;

        /* If we already touched the next --exec, step back */
        if (*argv && !strcmp(*argv, "--exec"))
            argv--;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
