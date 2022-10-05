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
    user_global->topo_debug = -1;

    user_global->demux = NULL;
    user_global->iface = NULL;

    user_global->enablex = -1;
    user_global->debug = -1;
    user_global->usize = HYD_USIZE_UNSET;

    user_global->auto_cleanup = -1;
    user_global->pmi_port = -1;
    user_global->skip_launch_node = -1;
    user_global->gpus_per_proc = HYD_GPUS_PER_PROC_UNSET;
    user_global->gpu_subdevs_per_proc = HYD_GPUS_PER_PROC_UNSET;
    user_global->singleton_port = 0;
    user_global->singleton_pid = 0;

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

static HYD_status alloc_proxy_list(struct HYD_node *node_list, int pgid,
                                   int *count_p, struct HYD_proxy **proxy_list_p)
{
    HYD_status status = HYD_SUCCESS;

    struct HYD_proxy *proxy_list = NULL;
    int count = 0;

    for (struct HYD_node * node = node_list; node; node = node->next) {
        count++;
    }
    HYDU_MALLOC_OR_JUMP(proxy_list, struct HYD_proxy *, count * sizeof(struct HYD_proxy), status);

    int i;
    i = 0;
    for (struct HYD_node * node = node_list; node; node = node->next) {
        struct HYD_proxy *proxy = &proxy_list[i];
        proxy->node = node;
        proxy->pgid = pgid;

        proxy->proxy_id = i;
        proxy->exec_launch_info = NULL;

        proxy->proxy_process_count = 0;
        proxy->filler_processes = 0;

        proxy->pid = NULL;
        proxy->exit_status = NULL;
        proxy->control_fd = HYD_FD_UNSET;

        proxy->exec_list = NULL;

        i++;
    }

  fn_exit:
    *count_p = count;
    *proxy_list_p = proxy_list;
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

static HYD_status prune_proxy_list(int *count_p, struct HYD_proxy **proxy_list_p, int extra_count)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    int proxy_count = *count_p;
    struct HYD_proxy *proxy_list = *proxy_list_p;

    int num_empty;
    num_empty = 0;
    for (int i = 0; i < proxy_count; i++) {
        if (proxy_list[i].proxy_process_count == 0) {
            num_empty++;
        }
    }
    /* extra_count is the extra empty processes after proxy_count. Realloc if non-zero. */
    if (num_empty > 0 || extra_count) {
        struct HYD_proxy *new_list;
        HYDU_MALLOC_OR_JUMP(new_list, struct HYD_proxy *,
                            (proxy_count - num_empty) * sizeof(struct HYD_proxy), status);
        int j = 0;
        for (int i = 0; i < proxy_count; i++) {
            if (proxy_list[i].proxy_process_count > 0) {
                new_list[j] = proxy_list[i];
                new_list[j].proxy_id = j;
                j++;
            }
        }
        MPL_free(proxy_list);
        *proxy_list_p = new_list;
        *count_p = proxy_count - num_empty;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

void HYDU_free_proxy_list(struct HYD_proxy *proxy_list, int count)
{
    HYDU_FUNC_ENTER();

    for (int i = 0; i < count; i++) {
        struct HYD_proxy *proxy = &proxy_list[i];

        proxy->node = NULL;

        if (proxy->exec_launch_info) {
            HYDU_free_strlist(proxy->exec_launch_info);
            MPL_free(proxy->exec_launch_info);
        }

        MPL_free(proxy->pid);
        MPL_free(proxy->exit_status);

        HYDU_free_exec_list(proxy->exec_list);
    }

    MPL_free(proxy_list);
    HYDU_FUNC_EXIT();
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

static HYD_status add_exec_to_proxy(struct HYD_exec *exec, struct HYD_proxy *proxy, int num_procs)
{
    int i;
    struct HYD_exec *texec;
    HYD_status status = HYD_SUCCESS;

    if (proxy->exec_list == NULL) {
        status = HYDU_alloc_exec(&proxy->exec_list);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        for (i = 0; exec->exec[i]; i++)
            proxy->exec_list->exec[i] = MPL_strdup(exec->exec[i]);
        proxy->exec_list->exec[i] = NULL;

        proxy->exec_list->wdir = exec->wdir ? MPL_strdup(exec->wdir) : NULL;
        proxy->exec_list->proc_count = num_procs;
        proxy->exec_list->env_prop = exec->env_prop ? MPL_strdup(exec->env_prop) : NULL;
        proxy->exec_list->user_env = HYDU_env_list_dup(exec->user_env);
        proxy->exec_list->appnum = exec->appnum;
    } else {
        for (texec = proxy->exec_list; texec->next; texec = texec->next);
        status = HYDU_alloc_exec(&texec->next);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        texec = texec->next;

        for (i = 0; exec->exec[i]; i++)
            texec->exec[i] = MPL_strdup(exec->exec[i]);
        texec->exec[i] = NULL;

        texec->wdir = exec->wdir ? MPL_strdup(exec->wdir) : NULL;
        texec->proc_count = num_procs;
        texec->env_prop = exec->env_prop ? MPL_strdup(exec->env_prop) : NULL;
        texec->user_env = HYDU_env_list_dup(exec->user_env);
        texec->appnum = exec->appnum;
    }
    proxy->proxy_process_count += num_procs;
    proxy->node->active_processes += num_procs;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_create_proxy_list_singleton(struct HYD_node *node, int pgid,
                                            int *proxy_count_p, struct HYD_proxy **proxy_list_p)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_proxy *proxy;
    int proxy_count;
    status = alloc_proxy_list(node, pgid, &proxy_count, &proxy);
    HYDU_ERR_POP(status, "error allocating proxy\n");

    proxy->proxy_process_count = 1;
    proxy->node->active_processes = 1;

    *proxy_list_p = proxy;
    *proxy_count_p = 1;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_create_proxy_list(int count, struct HYD_exec *exec_list, struct HYD_node *node_list,
                                  int pgid, int *proxy_count_p, struct HYD_proxy **proxy_list_p)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (exec_list == NULL) {
        status = HYD_FAILURE;
        HYDU_ERR_POP(status, "Missing executables\n");
    }

    /* Find the current maximum oversubscription on the nodes */
    int max_oversubscribe = 1;
    for (struct HYD_node * node = node_list; node; node = node->next) {
        int c = HYDU_dceil(node->active_processes, node->core_count);
        if (c > max_oversubscribe)
            max_oversubscribe = c;
    }

    /* make sure there are non-zero cores available */
    bool has_non_zero_cores = false;
    for (struct HYD_node * node = node_list; node; node = node->next) {
        if ((node->core_count * max_oversubscribe) - node->active_processes > 0) {;
            has_non_zero_cores = true;
            break;
        }
    }
    if (!has_non_zero_cores) {
        max_oversubscribe++;
    }

    struct HYD_proxy *proxy_list = NULL;
    int proxy_count = 0;
    status = alloc_proxy_list(node_list, pgid, &proxy_count, &proxy_list);
    HYDU_ERR_POP(status, "error allocating proxy_list\n");

    int extra_proxy_count = 0;
    int allocated_procs = 0;
    int dummy_fillers = 1;
    for (int i = 0; i < proxy_count; i++) {
        struct HYD_proxy *proxy = &proxy_list[i];
        struct HYD_node *node = proxy_list[i].node;

        int c = (node->core_count * max_oversubscribe) - node->active_processes;
        proxy->filler_processes = c;
        allocated_procs += c;

        if (proxy->filler_processes < node->core_count)
            dummy_fillers = 0;

        if (allocated_procs >= count) {
            extra_proxy_count = proxy_count - (i + 1);
            proxy_count = i + 1;
            break;
        }
    }

    /* If all proxies have as many filler processes as the number of
     * cores, we can reduce those filler processes */
    if (dummy_fillers) {
        for (int i = 0; i < proxy_count; i++) {
            proxy_list[i].filler_processes -= proxy_list[i].node->core_count;
        }
    }

    /* Proxy list is created; add the executables to the proxy list */
    if (proxy_count == 1) {
        /* Special case: there is only one proxy, so all executables
         * directly get appended to this proxy */
        for (struct HYD_exec * exec = exec_list; exec; exec = exec->next) {
            status = add_exec_to_proxy(exec, &proxy_list[0], exec->proc_count);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");
        }
    } else {
        struct HYD_exec *exec = exec_list;

        int i_proxy = 0;
        int filler_round = 0;
        for (int i = 0; i < proxy_count; i++) {
            if (proxy_list[i].filler_processes > 0) {
                i_proxy = i;
                filler_round = 1;
                break;
            }
        }
        struct HYD_proxy *proxy;
        proxy = &proxy_list[i_proxy];

        int exec_rem_procs = exec->proc_count;
        int proxy_rem_cores = filler_round ? proxy->filler_processes : proxy->node->core_count;

        while (exec) {
            if (exec_rem_procs == 0) {
                exec = exec->next;
                if (exec)
                    exec_rem_procs = exec->proc_count;
                else
                    break;
            }
            HYDU_ASSERT(exec_rem_procs, status);

            while (proxy_rem_cores == 0) {
                i_proxy++;
                if (i_proxy == proxy_count) {
                    i_proxy = 0;
                    filler_round = 0;
                }
                proxy = &proxy_list[i_proxy];

                proxy_rem_cores = filler_round ? proxy->filler_processes : proxy->node->core_count;
            }

            int num_procs;
            num_procs = (exec_rem_procs > proxy_rem_cores) ? proxy_rem_cores : exec_rem_procs;
            HYDU_ASSERT(num_procs, status);

            exec_rem_procs -= num_procs;
            proxy_rem_cores -= num_procs;

            status = add_exec_to_proxy(exec, proxy, num_procs);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");
        }
    }

    /* We need remove the empty proxies */
    status = prune_proxy_list(&proxy_count, &proxy_list, extra_proxy_count);
    HYDU_ERR_POP(status, "error in prune_proxy_list\n");

    *proxy_count_p = proxy_count;
    *proxy_list_p = proxy_list;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
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
