/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra.h"

void HYDU_init_user_global(struct HYD_user_global *user_global)
{
    user_global->rmk = NULL;
    user_global->launcher = NULL;
    user_global->launcher_exec = NULL;

    user_global->binding = NULL;
    user_global->topolib = NULL;

    user_global->demux = NULL;
    user_global->iface = NULL;

    user_global->enablex = -1;
    user_global->usize = HYD_USIZE_UNSET;

    user_global->auto_cleanup = -1;
    user_global->pmi_port = -1;
    user_global->skip_launch_node = -1;
    user_global->gpus_per_proc = HYD_GPUS_PER_PROC_UNSET;
    user_global->gpu_subdevs_per_proc = HYD_GPUS_PER_PROC_UNSET;

    HYDU_init_global_env(&user_global->global_env);
}

void HYDU_finalize_user_global(struct HYD_user_global *user_global)
{
    MPL_free(user_global->rmk);
    MPL_free(user_global->launcher);
    MPL_free(user_global->launcher_exec);
    MPL_free(user_global->binding);
    MPL_free(user_global->topolib);
    MPL_free(user_global->demux);
    MPL_free(user_global->iface);

    HYDU_finalize_global_env(&user_global->global_env);
}

void HYDU_init_global_env(struct HYD_env_global *global_env)
{
    global_env->system = NULL;
    global_env->user = NULL;
    global_env->inherited = NULL;
    global_env->prop = NULL;
}

void HYDU_finalize_global_env(struct HYD_env_global *global_env)
{
    if (global_env->system)
        HYDU_env_free_list(global_env->system);

    if (global_env->user)
        HYDU_env_free_list(global_env->user);

    if (global_env->inherited)
        HYDU_env_free_list(global_env->inherited);

    MPL_free(global_env->prop);
}

HYD_status HYDU_alloc_node(struct HYD_node **node)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*node, struct HYD_node *, sizeof(struct HYD_node), status);
    (*node)->hostname = NULL;
    (*node)->core_count = 0;
    (*node)->active_processes = 0;
    (*node)->node_id = -1;
    (*node)->control_fd = -1;
    (*node)->control_fd_refcnt = 0;
    (*node)->user = NULL;
    (*node)->local_binding = NULL;
    (*node)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_free_node_list(struct HYD_node *node_list)
{
    struct HYD_node *node, *tnode;

    node = node_list;
    while (node) {
        tnode = node->next;

        MPL_free(node->hostname);
        MPL_free(node->user);
        MPL_free(node->local_binding);
        MPL_free(node);

        node = tnode;
    }
}

HYD_status HYDU_alloc_exec(struct HYD_exec **exec)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*exec, struct HYD_exec *, sizeof(struct HYD_exec), status);
    (*exec)->exec[0] = NULL;
    (*exec)->wdir = NULL;
    (*exec)->proc_count = -1;
    (*exec)->env_prop = NULL;
    (*exec)->user_env = NULL;
    (*exec)->appnum = -1;
    (*exec)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_free_exec_list(struct HYD_exec *exec_list)
{
    struct HYD_exec *exec, *run;

    HYDU_FUNC_ENTER();

    exec = exec_list;
    while (exec) {
        run = exec->next;
        HYDU_free_strlist(exec->exec);

        MPL_free(exec->wdir);
        MPL_free(exec->env_prop);

        HYDU_env_free_list(exec->user_env);
        exec->user_env = NULL;

        MPL_free(exec);
        exec = run;
    }

    HYDU_FUNC_EXIT();
}

HYD_status HYDU_correct_wdir(char **wdir)
{
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (*wdir == NULL) {
        *wdir = HYDU_getcwd();
    } else if (*wdir[0] != '/') {
        tmp[0] = HYDU_getcwd();
        tmp[1] = MPL_strdup("/");
        tmp[2] = MPL_strdup(*wdir);
        tmp[3] = NULL;

        MPL_free(*wdir);
        status = HYDU_str_alloc_and_join(tmp, wdir);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(tmp);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
