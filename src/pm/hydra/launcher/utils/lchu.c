/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"

HYD_Handle handle;

HYD_Status HYD_LCHU_Create_host_list(void)
{
    FILE *fp = NULL;
    char line[2 * MAX_HOSTNAME_LEN], *hostname, *procs;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
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

    proc_params = handle.proc_params;
    while (proc_params) {
        if (!strcmp(handle.host_file, "HYDRA_USE_LOCALHOST")) {
            HYDU_Allocate_Partition(&proc_params->partition);
            proc_params->partition->name = MPIU_Strdup("localhost");
            proc_params->partition->proc_count = proc_params->exec_proc_count;
            total_procs = proc_params->exec_proc_count;
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
                    for (partition = proc_params->partition; partition->next;
                         partition = partition->next);
                    HYDU_Allocate_Partition(&partition->next);
                    partition = partition->next;
                }
                partition->name = MPIU_Strdup(hostname);
                partition->proc_count = num_procs;

                total_procs += num_procs;
                if (total_procs == proc_params->exec_proc_count)
                    break;
            }
        }

        if (total_procs != proc_params->exec_proc_count)
            break;
        proc_params = proc_params->next;
    }

    if (proc_params) {
        HYDU_Error_printf("Not enough number of hosts in host file: %s\n", handle.host_file);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

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
    struct HYD_Partition_list *partition;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {
            HYDU_FREE(partition->name);
            if (partition->mapping) {
                if (partition->mapping[i])
                    HYDU_FREE(partition->mapping[i]);
                HYDU_FREE(partition->mapping);
            }
        }
    }
    HYDU_FREE(handle.host_file);

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_LCHU_Create_env_list(void)
{
    struct HYD_Proc_params *proc_params;
    HYD_Env_t *env;
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
        proc_params = proc_params->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_LCHU_Free_env_list(void)
{
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_Env_free_list(handle.global_env);
    HYDU_Env_free_list(handle.system_env);
    HYDU_Env_free_list(handle.user_env);
    HYDU_Env_free_list(handle.prop_env);

    proc_params = handle.proc_params;
    while (proc_params) {
        HYDU_Env_free_list(proc_params->user_env);
        HYDU_Env_free_list(proc_params->prop_env);
        proc_params = proc_params->next;
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_LCHU_Free_io(void)
{
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {
            HYDU_FREE(partition->out);
            HYDU_FREE(partition->err);
        }
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_LCHU_Free_exec(void)
{
    struct HYD_Proc_params *proc_params;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = handle.proc_params;
    while (proc_params) {
        for (i = 0; proc_params->exec[i]; i++)
            HYDU_FREE(proc_params->exec[i]);
        proc_params = proc_params->next;
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_LCHU_Free_proc_params(void)
{
    struct HYD_Proc_params *proc_params, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proc_params = handle.proc_params;
    while (proc_params) {
        run = proc_params->next;
        HYDU_FREE(proc_params);
        proc_params = run;
    }

    HYDU_FUNC_EXIT();
    return status;
}
