/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

void HYDU_init_user_global(struct HYD_User_global *user_global)
{
    user_global->bootstrap = NULL;
    user_global->bootstrap_exec = NULL;

    user_global->binding = NULL;
    user_global->bindlib = NULL;

    user_global->ckpointlib = NULL;
    user_global->ckpoint_prefix = NULL;
    user_global->ckpoint_restart = 0;

    user_global->enablex = -1;
    user_global->debug = -1;
    user_global->wdir = NULL;
    user_global->launch_mode = HYD_LAUNCH_UNSET;

    HYDU_init_global_env(&user_global->global_env);
}

void HYDU_init_global_env(struct HYD_Env_global *global_env)
{
    global_env->system = NULL;
    global_env->user = NULL;
    global_env->inherited = NULL;
    global_env->prop = NULL;
}

HYD_Status HYDU_alloc_proxy(struct HYD_Proxy **proxy)
{
    static int proxy_id = 0;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*proxy, struct HYD_Proxy *, sizeof(struct HYD_Proxy), status);

    (*proxy)->hostname = NULL;
    (*proxy)->pid = -1;
    (*proxy)->in = -1;
    (*proxy)->out = -1;
    (*proxy)->err = -1;

    (*proxy)->proxy_id = proxy_id++;
    (*proxy)->active = 0;
    (*proxy)->exec_args = NULL;

    (*proxy)->segment_list = NULL;
    (*proxy)->proxy_core_count = 0;

    (*proxy)->exit_status = NULL;
    (*proxy)->control_fd = -1;

    (*proxy)->exec_list = NULL;
    (*proxy)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_alloc_exec_info(struct HYD_Exec_info **exec_info)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*exec_info, struct HYD_Exec_info *, sizeof(struct HYD_Exec_info), status);
    (*exec_info)->process_count = 0;
    (*exec_info)->exec[0] = NULL;
    (*exec_info)->user_env = NULL;
    (*exec_info)->env_prop = NULL;
    (*exec_info)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYDU_free_exec_info_list(struct HYD_Exec_info *exec_info_list)
{
    struct HYD_Exec_info *exec_info, *run;

    HYDU_FUNC_ENTER();

    exec_info = exec_info_list;
    while (exec_info) {
        run = exec_info->next;
        HYDU_free_strlist(exec_info->exec);

        if (exec_info->env_prop)
            HYDU_FREE(exec_info->env_prop);

        HYDU_env_free_list(exec_info->user_env);
        exec_info->user_env = NULL;

        HYDU_FREE(exec_info);
        exec_info = run;
    }

    HYDU_FUNC_EXIT();
}


void HYDU_free_proxy_list(struct HYD_Proxy *proxy_list)
{
    struct HYD_Proxy *proxy, *tproxy;
    struct HYD_Proxy_segment *segment, *tsegment;
    struct HYD_Proxy_exec *exec, *texec;

    HYDU_FUNC_ENTER();

    proxy = proxy_list;
    while (proxy) {
        tproxy = proxy->next;

        if (proxy->hostname)
            HYDU_FREE(proxy->hostname);
        if (proxy->exec_args) {
            HYDU_free_strlist(proxy->exec_args);
            HYDU_FREE(proxy->exec_args);
        }

        segment = proxy->segment_list;
        while (segment) {
            tsegment = segment->next;
            HYDU_FREE(segment);
            segment = tsegment;
        }

        if (proxy->exit_status)
            HYDU_FREE(proxy->exit_status);

        exec = proxy->exec_list;
        while (exec) {
            texec = exec->next;
            HYDU_free_strlist(exec->exec);
            if (exec->user_env)
                HYDU_env_free(exec->user_env);
            if (exec->env_prop)
                HYDU_FREE(exec->env_prop);
            HYDU_FREE(exec);
            exec = texec;
        }

        HYDU_FREE(proxy);
        proxy = tproxy;
    }

    HYDU_FUNC_EXIT();
}


HYD_Status HYDU_alloc_proxy_segment(struct HYD_Proxy_segment **segment)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*segment, struct HYD_Proxy_segment *,
                sizeof(struct HYD_Proxy_segment), status);
    (*segment)->start_pid = -1;
    (*segment)->proc_count = 0;
    (*segment)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYDU_alloc_proxy_exec(struct HYD_Proxy_exec **exec)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*exec, struct HYD_Proxy_exec *, sizeof(struct HYD_Proxy_exec), status);
    (*exec)->exec[0] = NULL;
    (*exec)->proc_count = 0;
    (*exec)->env_prop = NULL;
    (*exec)->user_env = NULL;
    (*exec)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
