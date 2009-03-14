/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "lchu.h"

HYD_Handle handle;

void HYD_LCHU_Init_params()
{
    handle.base_path = NULL;
    handle.proxy_port = -1;
    handle.bootstrap = NULL;

    handle.debug = -1;
    handle.enablex = -1;
    handle.wdir = NULL;
    handle.host_file = NULL;

    handle.global_env = NULL;
    handle.system_env = NULL;
    handle.user_env = NULL;
    handle.prop = HYD_ENV_PROP_UNSET;
    handle.prop_env = NULL;

    handle.in = -1;
    handle.stdin_cb = NULL;

    /* FIXME: Should the timers be initialized? */

    handle.proc_params = NULL;
    handle.func_depth = 0;
    handle.stdin_buf_offset = 0;
    handle.stdin_buf_count = 0;
}


void HYD_LCHU_Free_params()
{
    if (handle.base_path) {
        HYDU_FREE(handle.base_path);
        handle.base_path = NULL;
    }
    if (handle.bootstrap) {
        HYDU_FREE(handle.bootstrap);
        handle.bootstrap = NULL;
    }

    if (handle.wdir) {
        HYDU_FREE(handle.wdir);
        handle.wdir = NULL;
    }
    if (handle.host_file) {
        HYDU_FREE(handle.host_file);
        handle.host_file = NULL;
    }

    if (handle.global_env) {
        HYDU_Env_free_list(handle.global_env);
        handle.global_env = NULL;
    }

    if (handle.system_env) {
        HYDU_Env_free_list(handle.system_env);
        handle.system_env = NULL;
    }

    if (handle.user_env) {
        HYDU_Env_free_list(handle.user_env);
        handle.user_env = NULL;
    }

    if (handle.prop_env) {
        HYDU_Env_free_list(handle.prop_env);
        handle.prop_env = NULL;
    }

    if (handle.proc_params) {
        HYD_LCHU_Free_proc_params();
        handle.proc_params = NULL;
    }
}


void HYD_LCHU_Free_proc_params()
{
    struct HYD_Proc_params *proc_params, *run;

    HYDU_FUNC_ENTER();

    proc_params = handle.proc_params;
    while (proc_params) {
        run = proc_params->next;
        HYDU_Free_args(proc_params->exec);
        HYDU_Free_partition_list(proc_params->partition);
        proc_params->partition = NULL;

        HYDU_Env_free_list(proc_params->user_env);
        proc_params->user_env = NULL;
        HYDU_Env_free_list(proc_params->prop_env);
        proc_params->prop_env = NULL;

        HYDU_FREE(proc_params);
        proc_params = run;
    }

    HYDU_FUNC_EXIT();
}


HYD_Status HYD_LCHU_Create_host_list(void)
{
    FILE *fp = NULL;
    char line[2 * MAX_HOSTNAME_LEN], *hostname, *procs;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition, *run;
    int num_procs, total_procs;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (strcmp(handle.host_file, "HYDRA_USE_LOCALHOST")) {
        fp = fopen(handle.host_file, "r");
        if (fp == NULL) {
            HYDU_Error_printf("unable to open host file %s\n", handle.host_file);
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
    }

    HYDU_Debug("Partition list: ");
    proc_params = handle.proc_params;
    while (proc_params) {
        if (!strcmp(handle.host_file, "HYDRA_USE_LOCALHOST")) {
            HYDU_Allocate_Partition(&proc_params->partition);
            proc_params->partition->name = MPIU_Strdup("localhost");
            proc_params->partition->proc_count = proc_params->exec_proc_count;
            total_procs = proc_params->exec_proc_count;
            HYDU_Debug("%s:%d ", proc_params->partition->name, proc_params->exec_proc_count);
        }
        else {
            total_procs = 0;
            while (!feof(fp)) {
                if ((fscanf(fp, "%s", line) < 0) && errno) {
                    HYDU_Error_printf("unable to read input line (errno: %d)\n", errno);
                    status = HYD_INTERNAL_ERROR;
                    goto fn_fail;
                }
                if (feof(fp))
                    break;

                hostname = strtok(line, ":");
                procs = strtok(NULL, ":");

                num_procs = procs ? atoi(procs) : 1;
                if (num_procs > (proc_params->exec_proc_count - total_procs))
                    num_procs = (proc_params->exec_proc_count - total_procs);

                if (!proc_params->partition) {
                    HYDU_Allocate_Partition(&proc_params->partition);
                    partition = proc_params->partition;
                }
                else {
                    for (partition = proc_params->partition;
                         partition->next; partition = partition->next);
                    HYDU_Allocate_Partition(&partition->next);
                    partition = partition->next;
                }
                partition->name = MPIU_Strdup(hostname);
                partition->proc_count = num_procs;

                total_procs += num_procs;

                HYDU_Debug("%s:%d ", partition->name, partition->proc_count);

                if (total_procs == proc_params->exec_proc_count)
                    break;
            }
        }

        /* We don't have enough processes; use whatever we have */
        if (total_procs != proc_params->exec_proc_count) {
            for (partition = proc_params->partition;
                 partition->next; partition = partition->next);
            run = proc_params->partition;

            /* Optimize the case where there is only one node */
            if (run->next == NULL) {
                run->proc_count = proc_params->exec_proc_count;
                HYDU_Debug("%s:%d ", run->name, run->proc_count);
            }
            else {
                while (total_procs != proc_params->exec_proc_count) {
                    HYDU_Allocate_Partition(&partition->next);
                    partition = partition->next;
                    partition->name = MPIU_Strdup(run->name);
                    partition->proc_count = run->proc_count;

                    HYDU_Debug("%s:%d ", partition->name, partition->proc_count);

                    total_procs += partition->proc_count;
                    run = run->next;
                }
            }
        }

        proc_params = proc_params->next;
    }
    HYDU_Debug("\n");

  fn_exit:
    if (fp)
        fclose(fp);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_LCHU_Free_host_list(void)
{
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition, *next;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition;) {
            HYDU_FREE(partition->name);
            if (partition->mapping) {
                for (i = 0;; i++)
                    if (partition->mapping[i])
                        HYDU_FREE(partition->mapping[i]);
                HYDU_FREE(partition->mapping);
            }
            for (i = 0; partition->args[i]; i++)
                HYDU_FREE(partition->args[i]);
            next = partition->next;
            HYDU_FREE(partition);
            partition = next;
        }
    }
    HYDU_FREE(handle.host_file);

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_LCHU_Create_env_list(void)
{
    struct HYD_Proc_params *proc_params;
    HYD_Env_t *env, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.prop == HYD_ENV_PROP_ALL) {
        handle.prop_env = HYDU_Env_listdup(handle.global_env);
        for (env = handle.user_env; env; env = env->next) {
            status = HYDU_Env_add_to_list(&handle.prop_env, *env);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("unable to add env to list\n");
                goto fn_fail;
            }
        }
    }
    else if (handle.prop == HYD_ENV_PROP_LIST) {
        for (env = handle.user_env; env; env = env->next) {
            run = HYDU_Env_found_in_list(handle.global_env, *env);
            if (run) {
                status = HYDU_Env_add_to_list(&handle.prop_env, *run);
                if (status != HYD_SUCCESS) {
                    HYDU_Error_printf("unable to add env to list\n");
                    goto fn_fail;
                }
            }
        }
    }

    proc_params = handle.proc_params;
    while (proc_params) {
        if (proc_params->prop == HYD_ENV_PROP_ALL) {
            proc_params->prop_env = HYDU_Env_listdup(handle.global_env);
            for (env = proc_params->user_env; env; env = env->next) {
                status = HYDU_Env_add_to_list(&proc_params->prop_env, *env);
                if (status != HYD_SUCCESS) {
                    HYDU_Error_printf("unable to add env to list\n");
                    goto fn_fail;
                }
            }
        }
        else if (proc_params->prop == HYD_ENV_PROP_LIST) {
            for (env = proc_params->user_env; env; env = env->next) {
                run = HYDU_Env_found_in_list(handle.global_env, *env);
                if (run) {
                    status = HYDU_Env_add_to_list(&proc_params->prop_env, *run);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to add env to list\n");
                        goto fn_fail;
                    }
                }
            }
        }
        proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
