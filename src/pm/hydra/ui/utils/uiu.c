/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "uiu.h"

void HYD_uiu_init_params(void)
{
    HYDU_init_user_global(&HYD_handle.user_global);

    HYD_handle.base_path = NULL;
    HYD_handle.proxy_port = -1;

    HYD_handle.css = NULL;
    HYD_handle.rmk = NULL;

    HYD_handle.ckpoint_int = -1;

    HYD_handle.print_rank_map = -1;
    HYD_handle.print_all_exitcodes = -1;
    HYD_handle.pm_env = -1;

    HYD_handle.ranks_per_proc = -1;

    HYD_handle.stdin_cb = NULL;
    HYD_handle.stdout_cb = NULL;
    HYD_handle.stderr_cb = NULL;

    /* FIXME: Should the timers be initialized? */

    HYD_handle.node_list = NULL;
    HYD_handle.global_core_count = 0;

    HYD_handle.pg_list.proxy_list = NULL;
    HYD_handle.pg_list.pg_process_count = 0;
    HYD_handle.pg_list.next = NULL;

    HYD_handle.exec_info_list = NULL;

    HYD_handle.func_depth = 0;
    HYD_handle.stdin_buf_offset = 0;
    HYD_handle.stdin_buf_count = 0;
}


void HYD_uiu_free_params(void)
{
    if (HYD_handle.base_path)
        HYDU_FREE(HYD_handle.base_path);

    if (HYD_handle.user_global.bootstrap)
        HYDU_FREE(HYD_handle.user_global.bootstrap);

    if (HYD_handle.css)
        HYDU_FREE(HYD_handle.css);

    if (HYD_handle.rmk)
        HYDU_FREE(HYD_handle.rmk);

    if (HYD_handle.user_global.binding)
        HYDU_FREE(HYD_handle.user_global.binding);

    if (HYD_handle.user_global.bindlib)
        HYDU_FREE(HYD_handle.user_global.bindlib);

    if (HYD_handle.user_global.ckpointlib)
        HYDU_FREE(HYD_handle.user_global.ckpointlib);

    if (HYD_handle.user_global.ckpoint_prefix)
        HYDU_FREE(HYD_handle.user_global.ckpoint_prefix);

    if (HYD_handle.user_global.wdir)
        HYDU_FREE(HYD_handle.user_global.wdir);

    if (HYD_handle.user_global.bootstrap_exec)
        HYDU_FREE(HYD_handle.user_global.bootstrap_exec);

    if (HYD_handle.user_global.global_env.inherited)
        HYDU_env_free_list(HYD_handle.user_global.global_env.inherited);

    if (HYD_handle.user_global.global_env.system)
        HYDU_env_free_list(HYD_handle.user_global.global_env.system);

    if (HYD_handle.user_global.global_env.user)
        HYDU_env_free_list(HYD_handle.user_global.global_env.user);

    if (HYD_handle.user_global.global_env.prop)
        HYDU_FREE(HYD_handle.user_global.global_env.prop);

    if (HYD_handle.exec_info_list)
        HYDU_free_exec_info_list(HYD_handle.exec_info_list);

    if (HYD_handle.pg_list.proxy_list)
        HYDU_free_proxy_list(HYD_handle.pg_list.proxy_list);

    /* Re-initialize everything to default values */
    HYD_uiu_init_params();
}


HYD_status HYD_uiu_get_current_exec_info(struct HYD_exec_info **exec_info)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_handle.exec_info_list == NULL) {
        status = HYDU_alloc_exec_info(&HYD_handle.exec_info_list);
        HYDU_ERR_POP(status, "unable to allocate exec_info\n");
    }

    *exec_info = HYD_handle.exec_info_list;
    while ((*exec_info)->next)
        *exec_info = (*exec_info)->next;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_status add_exec_info_to_proxy(struct HYD_exec_info *exec_info,
                                         struct HYD_proxy *proxy, int num_procs)
{
    int i;
    struct HYD_proxy_exec *exec;
    HYD_status status = HYD_SUCCESS;

    if (proxy->exec_list == NULL) {
        status = HYDU_alloc_proxy_exec(&proxy->exec_list);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        for (i = 0; exec_info->exec[i]; i++)
            proxy->exec_list->exec[i] = HYDU_strdup(exec_info->exec[i]);
        proxy->exec_list->exec[i] = NULL;

        proxy->exec_list->proc_count = num_procs;
        proxy->exec_list->env_prop = exec_info->env_prop ?
            HYDU_strdup(exec_info->env_prop) : NULL;
        proxy->exec_list->user_env = HYDU_env_list_dup(exec_info->user_env);
    }
    else {
        for (exec = proxy->exec_list; exec->next; exec = exec->next);
        status = HYDU_alloc_proxy_exec(&exec->next);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        exec = exec->next;

        for (i = 0; exec_info->exec[i]; i++)
            exec->exec[i] = HYDU_strdup(exec_info->exec[i]);
        exec->exec[i] = NULL;

        exec->proc_count = num_procs;
        exec->env_prop = exec_info->env_prop ? HYDU_strdup(exec_info->env_prop) : NULL;
        exec->user_env = HYDU_env_list_dup(exec_info->user_env);
    }
    proxy->proxy_process_count += num_procs;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_uiu_merge_exec_info_to_proxy(void)
{
    int proxy_rem_procs, exec_rem_procs;
    struct HYD_proxy *proxy;
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_handle.pg_list.proxy_list;
    exec_info = HYD_handle.exec_info_list;
    proxy_rem_procs = proxy->node.core_count;
    exec_rem_procs = exec_info ? exec_info->process_count : 0;
    while (exec_info) {
        if (exec_rem_procs <= proxy_rem_procs) {
            status = add_exec_info_to_proxy(exec_info, proxy, exec_rem_procs);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");

            proxy_rem_procs -= exec_rem_procs;
            if (proxy_rem_procs == 0) {
                proxy = proxy->next;
                if (proxy == NULL)
                    proxy = HYD_handle.pg_list.proxy_list;
                proxy_rem_procs = proxy->node.core_count;
            }

            exec_info = exec_info->next;
            exec_rem_procs = exec_info ? exec_info->process_count : 0;
        }
        else {
            status = add_exec_info_to_proxy(exec_info, proxy, proxy_rem_procs);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");

            exec_rem_procs -= proxy_rem_procs;

            proxy = proxy->next;
            if (proxy == NULL)
                proxy = HYD_handle.pg_list.proxy_list;
            proxy_rem_procs = proxy->node.core_count;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYD_uiu_print_params(void)
{
    HYD_env_t *env;
    int i;
    struct HYD_proxy *proxy;
    struct HYD_proxy_exec *exec;
    struct HYD_exec_info *exec_info;

    HYDU_FUNC_ENTER();

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "mpiexec options:\n");
    HYDU_dump_noprefix(stdout, "----------------\n");
    HYDU_dump_noprefix(stdout, "  Base path: %s\n", HYD_handle.base_path);
    HYDU_dump_noprefix(stdout, "  Proxy port: %d\n", HYD_handle.proxy_port);
    HYDU_dump_noprefix(stdout, "  Bootstrap server: %s\n", HYD_handle.user_global.bootstrap);
    HYDU_dump_noprefix(stdout, "  Debug level: %d\n", HYD_handle.user_global.debug);
    HYDU_dump_noprefix(stdout, "  Enable X: %d\n", HYD_handle.user_global.enablex);
    HYDU_dump_noprefix(stdout, "  Working dir: %s\n", HYD_handle.user_global.wdir);

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "  Global environment:\n");
    HYDU_dump_noprefix(stdout, "  -------------------\n");
    for (env = HYD_handle.user_global.global_env.inherited; env; env = env->next)
        HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);

    if (HYD_handle.user_global.global_env.system) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  Hydra internal environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------------\n");
        for (env = HYD_handle.user_global.global_env.system; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    if (HYD_handle.user_global.global_env.user) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  User set environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------\n");
        for (env = HYD_handle.user_global.global_env.user; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_dump_noprefix(stdout, "    Executable information:\n");
    HYDU_dump_noprefix(stdout, "    **********************\n");
    i = 1;
    for (exec_info = HYD_handle.exec_info_list; exec_info; exec_info = exec_info->next) {
        HYDU_dump_noprefix(stdout, "      Executable ID: %2d\n", i++);
        HYDU_dump_noprefix(stdout, "      -----------------\n");
        HYDU_dump_noprefix(stdout, "        Process count: %d\n", exec_info->process_count);
        HYDU_dump_noprefix(stdout, "        Executable: ");
        HYDU_print_strlist(exec_info->exec);
        HYDU_dump_noprefix(stdout, "\n");

        if (exec_info->user_env) {
            HYDU_dump_noprefix(stdout, "\n");
            HYDU_dump_noprefix(stdout, "        User set environment:\n");
            HYDU_dump_noprefix(stdout, "        .....................\n");
            for (env = exec_info->user_env; env; env = env->next)
                HYDU_dump_noprefix(stdout, "          %s=%s\n", env->env_name, env->env_value);
        }
    }

    HYDU_dump_noprefix(stdout, "    Proxy information:\n");
    HYDU_dump_noprefix(stdout, "    *********************\n");
    i = 1;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        HYDU_dump_noprefix(stdout, "      Proxy ID: %2d\n", i++);
        HYDU_dump_noprefix(stdout, "      -----------------\n");
        HYDU_dump_noprefix(stdout, "        Proxy name: %s\n", proxy->node.hostname);
        HYDU_dump_noprefix(stdout, "        Process count: %d\n", proxy->node.core_count);
        HYDU_dump_noprefix(stdout, "        Start PID: %d\n", proxy->start_pid);
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "        Proxy exec list:\n");
        HYDU_dump_noprefix(stdout, "        ....................\n");
        for (exec = proxy->exec_list; exec; exec = exec->next)
            HYDU_dump_noprefix(stdout, "          Exec: %s; Process count: %d\n",
                               exec->exec[0], exec->proc_count);
    }

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_FUNC_EXIT();

    return;
}
