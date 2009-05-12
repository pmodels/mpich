/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "uiu.h"

HYD_Handle handle;

void HYD_UIU_init_params(void)
{
    handle.base_path = NULL;
    handle.proxy_port = -1;
    handle.launch_mode = HYD_LAUNCH_UNSET;

    handle.bootstrap = NULL;
    handle.css = NULL;
    handle.rmk = NULL;
    handle.binding = HYD_BIND_UNSET;
    handle.user_bind_map = NULL;

    handle.debug = -1;
    handle.print_rank_map = 0;
    handle.print_all_exitcodes = 0;
    handle.enablex = -1;
    handle.pm_env = -1;
    handle.wdir = NULL;
    handle.host_file = NULL;

    handle.ranks_per_proc = -1;
    handle.bootstrap_exec = NULL;

    handle.global_env = NULL;
    handle.system_env = NULL;
    handle.user_env = NULL;
    handle.prop = HYD_ENV_PROP_UNSET;
    handle.prop_env = NULL;

    handle.stdin_cb = NULL;
    handle.stdout_cb = NULL;
    handle.stderr_cb = NULL;

    /* FIXME: Should the timers be initialized? */

    handle.global_core_count = 0;
    handle.exec_info_list = NULL;
    handle.partition_list = NULL;

    handle.func_depth = 0;
    handle.stdin_buf_offset = 0;
    handle.stdin_buf_count = 0;
}


void HYD_UIU_free_params(void)
{
    if (handle.base_path)
        HYDU_FREE(handle.base_path);

    if (handle.bootstrap)
        HYDU_FREE(handle.bootstrap);

    if (handle.css)
        HYDU_FREE(handle.css);

    if (handle.rmk)
        HYDU_FREE(handle.rmk);

    if (handle.wdir)
        HYDU_FREE(handle.wdir);

    if (handle.host_file)
        HYDU_FREE(handle.host_file);

    if (handle.bootstrap_exec)
        HYDU_FREE(handle.bootstrap_exec);

    if (handle.global_env)
        HYDU_env_free_list(handle.global_env);

    if (handle.system_env)
        HYDU_env_free_list(handle.system_env);

    if (handle.user_env)
        HYDU_env_free_list(handle.user_env);

    if (handle.prop_env)
        HYDU_env_free_list(handle.prop_env);

    if (handle.exec_info_list)
        HYDU_free_exec_info_list(handle.exec_info_list);

    if (handle.partition_list)
        HYDU_free_partition_list(handle.partition_list);

    /* Re-initialize everything to default values */
    HYD_UIU_init_params();
}


HYD_Status HYD_UIU_create_env_list(void)
{
    struct HYD_Exec_info *exec_info;
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
    else if (handle.prop == HYD_ENV_PROP_NONE) {
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
    else if (handle.prop == HYD_ENV_PROP_UNSET) {
        for (env = handle.user_env; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &handle.prop_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }
    }

    exec_info = handle.exec_info_list;
    while (exec_info) {
        if (exec_info->prop == HYD_ENV_PROP_ALL) {
            exec_info->prop_env = HYDU_env_list_dup(handle.global_env);
            for (env = exec_info->user_env; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &exec_info->prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (exec_info->prop == HYD_ENV_PROP_NONE) {
            for (env = exec_info->user_env; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &exec_info->prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (exec_info->prop == HYD_ENV_PROP_LIST) {
            for (env = exec_info->user_env; env; env = env->next) {
                run = HYDU_env_lookup(*env, handle.global_env);
                if (run) {
                    status = HYDU_append_env_to_list(*run, &exec_info->prop_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                }
            }
        }
        else if (exec_info->prop == HYD_ENV_PROP_UNSET) {
            for (env = exec_info->user_env; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &exec_info->prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        exec_info = exec_info->next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_UIU_get_current_exec_info(struct HYD_Exec_info **info)
{
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.exec_info_list == NULL) {
        status = HYDU_alloc_exec_info(&handle.exec_info_list);
        HYDU_ERR_POP(status, "unable to allocate exec_info\n");
    }

    exec_info = handle.exec_info_list;
    while (exec_info->next)
        exec_info = exec_info->next;

    *info = exec_info;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_UIU_merge_exec_info_to_partition(void)
{
    int run_count, i, rem;
    struct HYD_Partition *partition;
    struct HYD_Exec_info *exec_info;
    struct HYD_Partition_exec *exec;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (partition = handle.partition_list; partition; partition = partition->next)
        handle.global_core_count += partition->partition_core_count;

    for (exec_info = handle.exec_info_list; exec_info; exec_info = exec_info->next) {
        /* The run_count tells us how many processes the partitions
         * before us can host */
        run_count = 0;
        for (partition = handle.partition_list; partition; partition = partition->next) {
            if (run_count >= exec_info->exec_proc_count)
                break;

            if (partition->exec_list == NULL) {
                status = HYDU_alloc_partition_exec(&partition->exec_list);
                HYDU_ERR_POP(status, "unable to allocate partition exec\n");

                partition->exec_list->pgid = 0; /* This is the COMM_WORLD exec */

                for (i = 0; exec_info->exec[i]; i++)
                    partition->exec_list->exec[i] = HYDU_strdup(exec_info->exec[i]);
                partition->exec_list->exec[i] = NULL;

                partition->exec_list->proc_count =
                    ((exec_info->exec_proc_count / handle.global_core_count) *
                     partition->partition_core_count);
                rem = (exec_info->exec_proc_count % handle.global_core_count);
                if (rem > run_count + partition->partition_core_count)
                    rem = run_count + partition->partition_core_count;
                partition->exec_list->proc_count += (rem > run_count) ? (rem - run_count) : 0;

                partition->exec_list->prop = exec_info->prop;
                partition->exec_list->prop_env = HYDU_env_list_dup(exec_info->prop_env);
            }
            else {
                for (exec = partition->exec_list; exec->next; exec = exec->next);
                status = HYDU_alloc_partition_exec(&exec->next);
                HYDU_ERR_POP(status, "unable to allocate partition exec\n");

                exec = exec->next;
                exec->pgid = 0; /* This is the COMM_WORLD exec */

                for (i = 0; exec_info->exec[i]; i++)
                    exec->exec[i] = HYDU_strdup(exec_info->exec[i]);
                exec->exec[i] = NULL;

                exec->proc_count =
                    ((exec_info->exec_proc_count / handle.global_core_count) *
                     partition->partition_core_count);
                rem = (exec_info->exec_proc_count % handle.global_core_count);
                if (rem > run_count + partition->partition_core_count)
                    rem = run_count + partition->partition_core_count;
                exec->proc_count += (rem > run_count) ? (rem - run_count) : 0;

                exec->prop = exec_info->prop;
                exec->prop_env = HYDU_env_list_dup(exec_info->prop_env);
            }

            run_count += partition->partition_core_count;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYD_UIU_print_params(void)
{
    HYD_Env_t *env;
    int i;
    struct HYD_Partition *partition;
    struct HYD_Partition_segment *segment;
    struct HYD_Partition_exec *exec;
    struct HYD_Exec_info *exec_info;

    HYDU_FUNC_ENTER();

    HYDU_Dump("\n");
    HYDU_Dump("=================================================");
    HYDU_Dump("=================================================");
    HYDU_Dump("\n");
    HYDU_Dump("mpiexec options:\n");
    HYDU_Dump("----------------\n");
    HYDU_Dump("  Base path: %s\n", handle.base_path);
    HYDU_Dump("  Proxy port: %d\n", handle.proxy_port);
    HYDU_Dump("  Bootstrap server: %s\n", handle.bootstrap);
    HYDU_Dump("  Debug level: %d\n", handle.debug);
    HYDU_Dump("  Enable X: %d\n", handle.enablex);
    HYDU_Dump("  Working dir: %s\n", handle.wdir);
    HYDU_Dump("  Host file: %s\n", handle.host_file);

    HYDU_Dump("\n");
    HYDU_Dump("  Global environment:\n");
    HYDU_Dump("  -------------------\n");
    for (env = handle.global_env; env; env = env->next)
        HYDU_Dump("    %s=%s\n", env->env_name, env->env_value);

    if (handle.system_env) {
        HYDU_Dump("\n");
        HYDU_Dump("  Hydra internal environment:\n");
        HYDU_Dump("  ---------------------------\n");
        for (env = handle.system_env; env; env = env->next)
            HYDU_Dump("    %s=%s\n", env->env_name, env->env_value);
    }

    if (handle.user_env) {
        HYDU_Dump("\n");
        HYDU_Dump("  User set environment:\n");
        HYDU_Dump("  ---------------------\n");
        for (env = handle.user_env; env; env = env->next)
            HYDU_Dump("    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_Dump("\n\n");

    HYDU_Dump("    Executable information:\n");
    HYDU_Dump("    **********************\n");
    i = 1;
    for (exec_info = handle.exec_info_list; exec_info; exec_info = exec_info->next) {
        HYDU_Dump("      Executable ID: %2d\n", i++);
        HYDU_Dump("      -----------------\n");
        HYDU_Dump("        Process count: %d\n", exec_info->exec_proc_count);
        HYDU_Dump("        Executable: ");
        HYDU_print_strlist(exec_info->exec);
        HYDU_Dump("\n");

        if (exec_info->user_env) {
            HYDU_Dump("\n");
            HYDU_Dump("        User set environment:\n");
            HYDU_Dump("        .....................\n");
            for (env = exec_info->user_env; env; env = env->next)
                HYDU_Dump("          %s=%s\n", env->env_name, env->env_value);
        }
    }

    HYDU_Dump("    Partition information:\n");
    HYDU_Dump("    *********************\n");
    i = 1;
    for (partition = handle.partition_list; partition; partition = partition->next) {
        HYDU_Dump("      Partition ID: %2d\n", i++);
        HYDU_Dump("      -----------------\n");
        HYDU_Dump("        Partition name: %s\n", partition->base->name);
        HYDU_Dump("        Process count: %d\n", partition->partition_core_count);
        HYDU_Dump("\n");
        HYDU_Dump("        Partition segment list:\n");
        HYDU_Dump("        .......................\n");
        for (segment = partition->segment_list; segment; segment = segment->next)
            HYDU_Dump("          Start PID: %d; Process count: %d\n",
                      segment->start_pid, segment->proc_count);
        HYDU_Dump("\n");
        HYDU_Dump("        Partition exec list:\n");
        HYDU_Dump("        ....................\n");
        for (exec = partition->exec_list; exec; exec = exec->next)
            HYDU_Dump("          Exec: %s; Process count: %d\n", exec->exec[0],
                      exec->proc_count);
    }

    HYDU_Dump("\n");
    HYDU_Dump("=================================================");
    HYDU_Dump("=================================================");
    HYDU_Dump("\n\n");

    HYDU_FUNC_EXIT();

    return;
}
