/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

    user_global->enablex = -1;
    user_global->debug = -1;
    user_global->usize = HYD_USIZE_UNSET;

    user_global->auto_cleanup = -1;

    HYDU_init_global_env(&user_global->global_env);
}

void HYDU_finalize_user_global(struct HYD_user_global *user_global)
{
    if (user_global->rmk)
        MPL_free(user_global->rmk);

    if (user_global->launcher)
        MPL_free(user_global->launcher);

    if (user_global->launcher_exec)
        MPL_free(user_global->launcher_exec);

    if (user_global->binding)
        MPL_free(user_global->binding);

    if (user_global->topolib)
        MPL_free(user_global->topolib);

    if (user_global->demux)
        MPL_free(user_global->demux);

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

    if (global_env->prop)
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
    (*node)->username = NULL;
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

        if (node->hostname)
            MPL_free(node->hostname);

        if (node->username)
            MPL_free(node->username);

        if (node->local_binding)
            MPL_free(node->local_binding);

        MPL_free(node);

        node = tnode;
    }
}

void HYDU_init_pg(struct HYD_pg *pg, int pgid)
{
    pg->pgid = pgid;
    pg->proxy_list = NULL;
    pg->proxy_count = 0;
    pg->pg_process_count = 0;
    pg->barrier_count = 0;
    pg->spawner_pg = NULL;
    pg->user_node_list = NULL;
    pg->pg_core_count = 0;
    pg->pg_scratch = NULL;
    pg->next = NULL;
}

HYD_status HYDU_alloc_pg(struct HYD_pg **pg, int pgid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*pg, struct HYD_pg *, sizeof(struct HYD_pg), status);
    HYDU_init_pg(*pg, pgid);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_free_pg_list(struct HYD_pg *pg_list)
{
    struct HYD_pg *pg, *tpg;

    pg = pg_list;
    while (pg) {
        tpg = pg->next;

        if (pg->proxy_list)
            HYDU_free_proxy_list(pg->proxy_list);

        if (pg->user_node_list)
            HYDU_free_node_list(pg->user_node_list);

        MPL_free(pg);

        pg = tpg;
    }
}

static HYD_status alloc_proxy(struct HYD_proxy **proxy, struct HYD_pg *pg, struct HYD_node *node)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*proxy, struct HYD_proxy *, sizeof(struct HYD_proxy), status);

    (*proxy)->node = node;
    (*proxy)->pg = pg;

    (*proxy)->proxy_id = -1;
    (*proxy)->exec_launch_info = NULL;

    (*proxy)->proxy_process_count = 0;
    (*proxy)->filler_processes = 0;

    (*proxy)->pid = NULL;
    (*proxy)->exit_status = NULL;
    (*proxy)->control_fd = HYD_FD_UNSET;

    (*proxy)->exec_list = NULL;

    (*proxy)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_free_proxy_list(struct HYD_proxy *proxy_list)
{
    struct HYD_proxy *proxy, *tproxy;

    HYDU_FUNC_ENTER();

    proxy = proxy_list;
    while (proxy) {
        tproxy = proxy->next;

        proxy->node = NULL;

        if (proxy->exec_launch_info) {
            HYDU_free_strlist(proxy->exec_launch_info);
            MPL_free(proxy->exec_launch_info);
        }

        if (proxy->pid)
            MPL_free(proxy->pid);

        if (proxy->exit_status)
            MPL_free(proxy->exit_status);

        HYDU_free_exec_list(proxy->exec_list);

        MPL_free(proxy);
        proxy = tproxy;
    }

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

        if (exec->wdir)
            MPL_free(exec->wdir);

        if (exec->env_prop)
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

        proxy->exec_list->wdir = MPL_strdup(exec->wdir);
        proxy->exec_list->proc_count = num_procs;
        proxy->exec_list->env_prop = exec->env_prop ? MPL_strdup(exec->env_prop) : NULL;
        proxy->exec_list->user_env = HYDU_env_list_dup(exec->user_env);
        proxy->exec_list->appnum = exec->appnum;
    }
    else {
        for (texec = proxy->exec_list; texec->next; texec = texec->next);
        status = HYDU_alloc_exec(&texec->next);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        texec = texec->next;

        for (i = 0; exec->exec[i]; i++)
            texec->exec[i] = MPL_strdup(exec->exec[i]);
        texec->exec[i] = NULL;

        texec->wdir = MPL_strdup(exec->wdir);
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

HYD_status HYDU_create_proxy_list(struct HYD_exec *exec_list, struct HYD_node *node_list,
                                  struct HYD_pg *pg)
{
    struct HYD_proxy *proxy = NULL, *last_proxy = NULL, *tmp;
    struct HYD_exec *exec;
    struct HYD_node *node;
    int max_oversubscribe, c, num_procs, proxy_rem_cores, exec_rem_procs, allocated_procs;
    int filler_round, num_nodes, i, dummy_fillers;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the current maximum oversubscription on the nodes */
    max_oversubscribe = 1;
    num_nodes = 0;
    for (node = node_list; node; node = node->next) {
        c = HYDU_dceil(node->active_processes, node->core_count);
        if (c > max_oversubscribe)
            max_oversubscribe = c;
        num_nodes++;
    }

    /* make sure there are non-zero cores available */
    c = 0;
    for (node = node_list; node; node = node->next)
        c += (node->core_count * max_oversubscribe) - node->active_processes;
    if (c == 0)
        max_oversubscribe++;

    allocated_procs = 0;
    dummy_fillers = 1;
    for (node = node_list; node; node = node->next) {
        /* check how many cores are available */
        c = (node->core_count * max_oversubscribe) - node->active_processes;

        /* create a proxy associated with this node */
        status = alloc_proxy(&proxy, pg, node);
        HYDU_ERR_POP(status, "error allocating proxy\n");

        proxy->filler_processes = c;
        allocated_procs += c;

        if (proxy->filler_processes < node->core_count)
            dummy_fillers = 0;

        if (pg->proxy_list == NULL)
            pg->proxy_list = proxy;
        else
            last_proxy->next = proxy;
        last_proxy = proxy;

        if (allocated_procs >= pg->pg_process_count)
            break;
    }

    /* If all proxies have as many filler processes as the number of
     * cores, we can reduce those filler processes */
    if (dummy_fillers)
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            proxy->filler_processes -= proxy->node->core_count;

    /* Proxy list is created; add the executables to the proxy list */
    if (pg->proxy_list->next == NULL) {
        /* Special case: there is only one proxy, so all executables
         * directly get appended to this proxy */
        for (exec = exec_list; exec; exec = exec->next) {
            status = add_exec_to_proxy(exec, pg->proxy_list, exec->proc_count);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");
        }
    }
    else {
        exec = exec_list;

        filler_round = 1;
        for (proxy = pg->proxy_list; proxy && proxy->filler_processes == 0; proxy = proxy->next);
        if (proxy == NULL) {
            filler_round = 0;
            proxy = pg->proxy_list;
        }

        exec_rem_procs = exec->proc_count;
        proxy_rem_cores = filler_round ? proxy->filler_processes : proxy->node->core_count;

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
                proxy = proxy->next;

                if (proxy == NULL) {
                    filler_round = 0;
                    proxy = pg->proxy_list;
                }

                proxy_rem_cores = filler_round ? proxy->filler_processes : proxy->node->core_count;
            }

            num_procs = (exec_rem_procs > proxy_rem_cores) ? proxy_rem_cores : exec_rem_procs;
            HYDU_ASSERT(num_procs, status);

            exec_rem_procs -= num_procs;
            proxy_rem_cores -= num_procs;

            status = add_exec_to_proxy(exec, proxy, num_procs);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");
        }
    }

    /* find dummy proxies and remove them */
    while (pg->proxy_list->exec_list == NULL) {
        tmp = pg->proxy_list->next;
        pg->proxy_list->next = NULL;
        HYDU_free_proxy_list(pg->proxy_list);
        pg->proxy_list = tmp;
    }
    for (proxy = pg->proxy_list; proxy->next;) {
        if (proxy->next->exec_list == NULL) {
            tmp = proxy->next;
            proxy->next = proxy->next->next;
            tmp->next = NULL;
            HYDU_free_proxy_list(tmp);
        }
        else {
            proxy = proxy->next;
        }
    }

    for (proxy = pg->proxy_list, i = 0; proxy; proxy = proxy->next, i++)
        proxy->proxy_id = i;
    pg->proxy_count = i;

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
    }
    else if (*wdir[0] != '/') {
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
