/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "lchu.h"

HYD_Handle handle;

void HYD_LCHU_init_params(void)
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


void HYD_LCHU_free_params(void)
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
        HYDU_env_free_list(handle.global_env);
        handle.global_env = NULL;
    }

    if (handle.system_env) {
        HYDU_env_free_list(handle.system_env);
        handle.system_env = NULL;
    }

    if (handle.user_env) {
        HYDU_env_free_list(handle.user_env);
        handle.user_env = NULL;
    }

    if (handle.prop_env) {
        HYDU_env_free_list(handle.prop_env);
        handle.prop_env = NULL;
    }

    if (handle.proc_params) {
        HYD_LCHU_free_proc_params();
        handle.proc_params = NULL;
    }
}


void HYD_LCHU_free_proc_params(void)
{
    struct HYD_Proc_params *proc_params, *run;

    HYDU_FUNC_ENTER();

    proc_params = handle.proc_params;
    while (proc_params) {
        run = proc_params->next;
        HYDU_free_strlist(proc_params->exec);
        HYDU_free_partition_list(proc_params->partition);
        proc_params->partition = NULL;

        HYDU_env_free_list(proc_params->user_env);
        proc_params->user_env = NULL;
        HYDU_env_free_list(proc_params->prop_env);
        proc_params->prop_env = NULL;

        HYDU_FREE(proc_params);
        proc_params = run;
    }

    HYDU_FUNC_EXIT();
}


HYD_Status HYD_LCHU_create_host_list(void)
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
        if (fp == NULL)
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                 "unable to open host file: %s\n", handle.host_file);
    }

    HYDU_Debug("Partition list: ");
    proc_params = handle.proc_params;
    while (proc_params) {
        if (!strcmp(handle.host_file, "HYDRA_USE_LOCALHOST")) {
            HYDU_alloc_partition(&proc_params->partition);
            proc_params->partition->name = MPIU_Strdup("localhost");
            proc_params->partition->proc_count = proc_params->exec_proc_count;
            total_procs = proc_params->exec_proc_count;
            HYDU_Debug("%s:%d ", proc_params->partition->name, proc_params->exec_proc_count);
        }
        else {
            total_procs = 0;
            while (!feof(fp)) {
                if ((fscanf(fp, "%s", line) < 0) && errno)
                    HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                         "unable to read input line (errno: %d)\n", errno);
                if (feof(fp))
                    break;

                hostname = strtok(line, ":");
                procs = strtok(NULL, ":");

                num_procs = procs ? atoi(procs) : 1;
                if (num_procs > (proc_params->exec_proc_count - total_procs))
                    num_procs = (proc_params->exec_proc_count - total_procs);

                if (!proc_params->partition) {
                    HYDU_alloc_partition(&proc_params->partition);
                    partition = proc_params->partition;
                }
                else {
                    for (partition = proc_params->partition;
                         partition->next; partition = partition->next);
                    HYDU_alloc_partition(&partition->next);
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
                    HYDU_alloc_partition(&partition->next);
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


HYD_Status HYD_LCHU_free_host_list(void)
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


HYD_Status HYD_LCHU_create_env_list(void)
{
    struct HYD_Proc_params *proc_params;
    HYD_Env_t *env, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.prop == HYD_ENV_PROP_ALL) {
        handle.prop_env = HYDU_env_list_dup(handle.global_env);
        for (env = handle.user_env; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &handle.prop_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }
    }
    else if (handle.prop == HYD_ENV_PROP_LIST) {
        for (env = handle.user_env; env; env = env->next) {
            run = HYDU_env_lookup(*env, handle.global_env);
            if (run) {
                status = HYDU_append_env_to_list(*run, &handle.prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
    }

    proc_params = handle.proc_params;
    while (proc_params) {
        if (proc_params->prop == HYD_ENV_PROP_ALL) {
            proc_params->prop_env = HYDU_env_list_dup(handle.global_env);
            for (env = proc_params->user_env; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &proc_params->prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (proc_params->prop == HYD_ENV_PROP_LIST) {
            for (env = proc_params->user_env; env; env = env->next) {
                run = HYDU_env_lookup(*env, handle.global_env);
                if (run) {
                    status = HYDU_append_env_to_list(*run, &proc_params->prop_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
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


void HYD_LCHU_print_params(void)
{
    HYD_Env_t *env;
    int i, j;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;

    HYDU_FUNC_ENTER();

    HYDU_Debug("\n");
    HYDU_Debug("=================================================");
    HYDU_Debug("=================================================");
    HYDU_Debug("\n");
    HYDU_Debug("mpiexec options:\n");
    HYDU_Debug("----------------\n");
    HYDU_Debug("  Base path: %s\n", handle.base_path);
    HYDU_Debug("  Proxy port: %d\n", handle.proxy_port);
    HYDU_Debug("  Bootstrap server: %s\n", handle.bootstrap);
    HYDU_Debug("  Debug level: %d\n", handle.debug);
    HYDU_Debug("  Enable X: %d\n", handle.enablex);
    HYDU_Debug("  Working dir: %s\n", handle.wdir);
    HYDU_Debug("  Host file: %s\n", handle.host_file);

    HYDU_Debug("\n");
    HYDU_Debug("  Global environment:\n");
    HYDU_Debug("  -------------------\n");
    for (env = handle.global_env; env; env = env->next)
        HYDU_Debug("    %s=%s\n", env->env_name, env->env_value);

    if (handle.system_env) {
        HYDU_Debug("\n");
        HYDU_Debug("  Hydra internal environment:\n");
        HYDU_Debug("  ---------------------------\n");
        for (env = handle.system_env; env; env = env->next)
            HYDU_Debug("    %s=%s\n", env->env_name, env->env_value);
    }

    if (handle.user_env) {
        HYDU_Debug("\n");
        HYDU_Debug("  User set environment:\n");
        HYDU_Debug("  ---------------------\n");
        for (env = handle.user_env; env; env = env->next)
            HYDU_Debug("    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_Debug("\n\n");

    HYDU_Debug("    Process parameters:\n");
    HYDU_Debug("    *******************\n");
    i = 1;
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        HYDU_Debug("      Executable ID: %2d\n", i++);
        HYDU_Debug("      -----------------\n");
        HYDU_Debug("        Process count: %d\n", proc_params->exec_proc_count);
        HYDU_Debug("        Executable: ");
        for (j = 0; proc_params->exec[j]; j++)
            HYDU_Debug("%s ", proc_params->exec[j]);
        HYDU_Debug("\n");

        if (proc_params->user_env) {
            HYDU_Debug("\n");
            HYDU_Debug("        User set environment:\n");
            HYDU_Debug("        .....................\n");
            for (env = proc_params->user_env; env; env = env->next)
                HYDU_Debug("          %s=%s\n", env->env_name, env->env_value);
        }

        j = 0;
        for (partition = proc_params->partition; partition; partition = partition->next) {
            HYDU_Debug("\n");
            HYDU_Debug("        Partition ID: %2d\n", j++);
            HYDU_Debug("        ----------------\n");
            HYDU_Debug("          Partition name: %s\n", partition->name);
            HYDU_Debug("          Partition process count: %d\n", partition->proc_count);
            HYDU_Debug("\n");
        }
    }

    HYDU_Debug("\n");
    HYDU_Debug("=================================================");
    HYDU_Debug("=================================================");
    HYDU_Debug("\n\n");

    HYDU_FUNC_EXIT();

    return;
}


HYD_Status HYD_LCHU_allocate_proc_params(struct HYD_Proc_params **params)
{
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(proc_params, struct HYD_Proc_params *, sizeof(struct HYD_Proc_params), status);

    proc_params->exec_proc_count = 0;
    proc_params->partition = NULL;

    proc_params->exec[0] = NULL;
    proc_params->user_env = NULL;
    proc_params->prop = HYD_ENV_PROP_UNSET;
    proc_params->prop_env = NULL;
    proc_params->stdout_cb = NULL;
    proc_params->stderr_cb = NULL;
    proc_params->next = NULL;

    *params = proc_params;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_LCHU_get_current_proc_params(struct HYD_Proc_params **params)
{
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.proc_params == NULL) {
        status = HYD_LCHU_allocate_proc_params(&handle.proc_params);
        HYDU_ERR_POP(status, "unable to allocate proc_params\n");
    }

    proc_params = handle.proc_params;
    while (proc_params->next)
        proc_params = proc_params->next;

    *params = proc_params;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
