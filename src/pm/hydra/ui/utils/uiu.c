/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "uiu.h"

void HYD_UIU_init_params(void)
{
    HYD_handle.base_path = NULL;
    HYD_handle.proxy_port = -1;
    HYD_handle.launch_mode = HYD_LAUNCH_UNSET;

    HYD_handle.bootstrap = NULL;
    HYD_handle.css = NULL;
    HYD_handle.rmk = NULL;
    HYD_handle.binding = HYD_BIND_UNSET;
    HYD_handle.bindlib = HYD_BINDLIB_UNSET;
    HYD_handle.user_bind_map = NULL;

    HYD_handle.ckpointlib = HYD_CKPOINTLIB_UNSET;
    HYD_handle.ckpoint_int = -1;
    HYD_handle.ckpoint_prefix = NULL;

    HYD_handle.debug = -1;
    HYD_handle.print_rank_map = -1;
    HYD_handle.print_all_exitcodes = -1;
    HYD_handle.enablex = -1;
    HYD_handle.pm_env = -1;
    HYD_handle.wdir = NULL;
    HYD_handle.host_file = NULL;

    HYD_handle.ranks_per_proc = -1;
    HYD_handle.bootstrap_exec = NULL;

    HYD_handle.inherited_env = NULL;
    HYD_handle.system_env = NULL;
    HYD_handle.user_env = NULL;
    HYD_handle.prop = HYD_ENV_PROP_UNSET;

    HYD_handle.stdin_cb = NULL;
    HYD_handle.stdout_cb = NULL;
    HYD_handle.stderr_cb = NULL;

    /* FIXME: Should the timers be initialized? */

    HYD_handle.global_core_count = 0;
    HYD_handle.exec_info_list = NULL;
    HYD_handle.partition_list = NULL;

    HYD_handle.func_depth = 0;
    HYD_handle.stdin_buf_offset = 0;
    HYD_handle.stdin_buf_count = 0;
}


void HYD_UIU_free_params(void)
{
    if (HYD_handle.base_path)
        HYDU_FREE(HYD_handle.base_path);

    if (HYD_handle.bootstrap)
        HYDU_FREE(HYD_handle.bootstrap);

    if (HYD_handle.css)
        HYDU_FREE(HYD_handle.css);

    if (HYD_handle.rmk)
        HYDU_FREE(HYD_handle.rmk);

    if (HYD_handle.user_bind_map)
        HYDU_FREE(HYD_handle.user_bind_map);

    if (HYD_handle.ckpoint_prefix)
        HYDU_FREE(HYD_handle.ckpoint_prefix);

    if (HYD_handle.wdir)
        HYDU_FREE(HYD_handle.wdir);

    if (HYD_handle.host_file)
        HYDU_FREE(HYD_handle.host_file);

    if (HYD_handle.bootstrap_exec)
        HYDU_FREE(HYD_handle.bootstrap_exec);

    if (HYD_handle.inherited_env)
        HYDU_env_free_list(HYD_handle.inherited_env);

    if (HYD_handle.system_env)
        HYDU_env_free_list(HYD_handle.system_env);

    if (HYD_handle.user_env)
        HYDU_env_free_list(HYD_handle.user_env);

    if (HYD_handle.exec_info_list)
        HYDU_free_exec_info_list(HYD_handle.exec_info_list);

    if (HYD_handle.partition_list)
        HYDU_free_partition_list(HYD_handle.partition_list);

    /* Re-initialize everything to default values */
    HYD_UIU_init_params();
}


HYD_Status HYD_UIU_create_env_list(void)
{
    struct HYD_Exec_info *exec_info;
    HYD_Env_t *env, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_handle.prop == HYD_ENV_PROP_LIST) {
        for (env = HYD_handle.user_env; env; env = env->next) {
            run = HYDU_env_lookup(*env, HYD_handle.inherited_env);
            if (run) {
                /* Dump back the updated environment to the user list */
                status = HYDU_append_env_to_list(*run, &HYD_handle.user_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
    }

    exec_info = HYD_handle.exec_info_list;
    while (exec_info) {
        if (exec_info->prop == HYD_ENV_PROP_LIST) {
            for (env = exec_info->user_env; env; env = env->next) {
                run = HYDU_env_lookup(*env, HYD_handle.inherited_env);
                if (run) {
                    /* Dump back the updated environment to the user list */
                    status = HYDU_append_env_to_list(*run, &exec_info->user_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                }
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

    if (HYD_handle.exec_info_list == NULL) {
        status = HYDU_alloc_exec_info(&HYD_handle.exec_info_list);
        HYDU_ERR_POP(status, "unable to allocate exec_info\n");
    }

    exec_info = HYD_handle.exec_info_list;
    while (exec_info->next)
        exec_info = exec_info->next;

    *info = exec_info;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status add_exec_info_to_partition(struct HYD_Exec_info *exec_info,
                                             struct HYD_Partition *partition,
                                             int num_procs)
{
    int i;
    struct HYD_Partition_exec *exec;
    HYD_Status status = HYD_SUCCESS;

    if (partition->exec_list == NULL) {
        status = HYDU_alloc_partition_exec(&partition->exec_list);
        HYDU_ERR_POP(status, "unable to allocate partition exec\n");

        partition->exec_list->pgid = 0; /* This is the COMM_WORLD exec */

        for (i = 0; exec_info->exec[i]; i++)
            partition->exec_list->exec[i] = HYDU_strdup(exec_info->exec[i]);
        partition->exec_list->exec[i] = NULL;

        partition->exec_list->proc_count = num_procs;
        partition->exec_list->prop = exec_info->prop;
        partition->exec_list->user_env = HYDU_env_list_dup(exec_info->user_env);
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

        exec->proc_count = num_procs;
        exec->prop = exec_info->prop;
        exec->user_env = HYDU_env_list_dup(exec_info->user_env);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_UIU_merge_exec_info_to_partition(void)
{
    int partition_rem_procs, exec_rem_procs;
    struct HYD_Partition *partition;
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (partition = HYD_handle.partition_list; partition; partition = partition->next)
        HYD_handle.global_core_count += partition->partition_core_count;

    partition = HYD_handle.partition_list;
    exec_info = HYD_handle.exec_info_list;
    partition_rem_procs = partition->partition_core_count;
    exec_rem_procs = exec_info ? exec_info->exec_proc_count : 0;
    while (exec_info) {
        if (exec_rem_procs <= partition_rem_procs) {
            status = add_exec_info_to_partition(exec_info, partition, exec_rem_procs);
            HYDU_ERR_POP(status, "unable to add executable to partition\n");

            partition_rem_procs -= exec_rem_procs;
            if (partition_rem_procs == 0) {
                partition = partition->next;
                if (partition == NULL)
                    partition = HYD_handle.partition_list;
                partition_rem_procs = partition->partition_core_count;
            }

            exec_info = exec_info->next;
            exec_rem_procs = exec_info ? exec_info->exec_proc_count : 0;
        }
        else {
            status = add_exec_info_to_partition(exec_info, partition, partition_rem_procs);
            HYDU_ERR_POP(status, "unable to add executable to partition\n");

            exec_rem_procs -= partition_rem_procs;

            partition = partition->next;
            if (partition == NULL)
                partition = HYD_handle.partition_list;
            partition_rem_procs = partition->partition_core_count;
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
    HYDU_Dump("  Base path: %s\n", HYD_handle.base_path);
    HYDU_Dump("  Proxy port: %d\n", HYD_handle.proxy_port);
    HYDU_Dump("  Bootstrap server: %s\n", HYD_handle.bootstrap);
    HYDU_Dump("  Debug level: %d\n", HYD_handle.debug);
    HYDU_Dump("  Enable X: %d\n", HYD_handle.enablex);
    HYDU_Dump("  Working dir: %s\n", HYD_handle.wdir);
    HYDU_Dump("  Host file: %s\n", HYD_handle.host_file);

    HYDU_Dump("\n");
    HYDU_Dump("  Global environment:\n");
    HYDU_Dump("  -------------------\n");
    for (env = HYD_handle.inherited_env; env; env = env->next)
        HYDU_Dump("    %s=%s\n", env->env_name, env->env_value);

    if (HYD_handle.system_env) {
        HYDU_Dump("\n");
        HYDU_Dump("  Hydra internal environment:\n");
        HYDU_Dump("  ---------------------------\n");
        for (env = HYD_handle.system_env; env; env = env->next)
            HYDU_Dump("    %s=%s\n", env->env_name, env->env_value);
    }

    if (HYD_handle.user_env) {
        HYDU_Dump("\n");
        HYDU_Dump("  User set environment:\n");
        HYDU_Dump("  ---------------------\n");
        for (env = HYD_handle.user_env; env; env = env->next)
            HYDU_Dump("    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_Dump("\n\n");

    HYDU_Dump("    Executable information:\n");
    HYDU_Dump("    **********************\n");
    i = 1;
    for (exec_info = HYD_handle.exec_info_list; exec_info; exec_info = exec_info->next) {
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
    for (partition = HYD_handle.partition_list; partition; partition = partition->next) {
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
