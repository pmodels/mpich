/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
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

    user_global->ckpointlib = NULL;
    user_global->ckpoint_prefix = NULL;
    user_global->ckpoint_num = -1;

    user_global->demux = NULL;
    user_global->iface = NULL;

    user_global->enablex = -1;
    user_global->debug = -1;

    user_global->auto_cleanup = -1;

    HYDU_init_global_env(&user_global->global_env);
}

void HYDU_finalize_user_global(struct HYD_user_global *user_global)
{
    if (user_global->rmk)
        HYDU_FREE(user_global->rmk);

    if (user_global->launcher)
        HYDU_FREE(user_global->launcher);

    if (user_global->launcher_exec)
        HYDU_FREE(user_global->launcher_exec);

    if (user_global->binding)
        HYDU_FREE(user_global->binding);

    if (user_global->topolib)
        HYDU_FREE(user_global->topolib);

    if (user_global->ckpointlib)
        HYDU_FREE(user_global->ckpointlib);

    if (user_global->ckpoint_prefix)
        HYDU_FREE(user_global->ckpoint_prefix);

    if (user_global->demux)
        HYDU_FREE(user_global->demux);

    if (user_global->iface)
        HYDU_FREE(user_global->iface);

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
        HYDU_FREE(global_env->prop);
}

HYD_status HYDU_alloc_node(struct HYD_node **node)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*node, struct HYD_node *, sizeof(struct HYD_node), status);
    (*node)->hostname = NULL;
    (*node)->core_count = 0;
    (*node)->active_processes = 0;
    (*node)->node_id = -1;
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

        if (node->hostname)
            HYDU_FREE(node->hostname);

        if (node->user)
            HYDU_FREE(node->user);

        if (node->local_binding)
            HYDU_FREE(node->local_binding);

        HYDU_FREE(node);

        node = tnode;
    }
}

void HYDU_init_pg(struct HYD_pg *pg, int pgid)
{
    pg->pgid = pgid;
    pg->proxy_list = NULL;
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

    HYDU_MALLOC(*pg, struct HYD_pg *, sizeof(struct HYD_pg), status);
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

        HYDU_FREE(pg);

        pg = tpg;
    }
}

static HYD_status alloc_proxy(struct HYD_proxy **proxy, struct HYD_pg *pg,
                              struct HYD_node *node)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*proxy, struct HYD_proxy *, sizeof(struct HYD_proxy), status);

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
            HYDU_FREE(proxy->exec_launch_info);
        }

        if (proxy->pid)
            HYDU_FREE(proxy->pid);

        if (proxy->exit_status)
            HYDU_FREE(proxy->exit_status);

        HYDU_free_exec_list(proxy->exec_list);

        HYDU_FREE(proxy);
        proxy = tproxy;
    }

    HYDU_FUNC_EXIT();
}

HYD_status HYDU_alloc_exec(struct HYD_exec **exec)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*exec, struct HYD_exec *, sizeof(struct HYD_exec), status);
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
            HYDU_FREE(exec->wdir);

        if (exec->env_prop)
            HYDU_FREE(exec->env_prop);

        HYDU_env_free_list(exec->user_env);
        exec->user_env = NULL;

        HYDU_FREE(exec);
        exec = run;
    }

    HYDU_FUNC_EXIT();
}

static HYD_status add_exec_to_proxy(struct HYD_exec *exec, struct HYD_proxy *proxy,
                                    int num_procs)
{
    int i;
    struct HYD_exec *texec;
    HYD_status status = HYD_SUCCESS;

    if (proxy->exec_list == NULL) {
        status = HYDU_alloc_exec(&proxy->exec_list);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        for (i = 0; exec->exec[i]; i++)
            proxy->exec_list->exec[i] = HYDU_strdup(exec->exec[i]);
        proxy->exec_list->exec[i] = NULL;

        proxy->exec_list->wdir = HYDU_strdup(exec->wdir);
        proxy->exec_list->proc_count = num_procs;
        proxy->exec_list->env_prop = exec->env_prop ? HYDU_strdup(exec->env_prop) : NULL;
        proxy->exec_list->user_env = HYDU_env_list_dup(exec->user_env);
        proxy->exec_list->appnum = exec->appnum;
    }
    else {
        for (texec = proxy->exec_list; texec->next; texec = texec->next);
        status = HYDU_alloc_exec(&texec->next);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");

        texec = texec->next;

        for (i = 0; exec->exec[i]; i++)
            texec->exec[i] = HYDU_strdup(exec->exec[i]);
        texec->exec[i] = NULL;

        texec->wdir = HYDU_strdup(exec->wdir);
        texec->proc_count = num_procs;
        texec->env_prop = exec->env_prop ? HYDU_strdup(exec->env_prop) : NULL;
        texec->user_env = HYDU_env_list_dup(exec->user_env);
        texec->appnum = exec->appnum;
    }
    proxy->proxy_process_count += num_procs;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_create_proxy_list(struct HYD_exec *exec_list, struct HYD_node *node_list,
                                  struct HYD_pg *pg)
{
    struct HYD_proxy *proxy = NULL, *last_proxy;
    struct HYD_exec *exec;
    struct HYD_node *node;
    int process_core_ratio, c, global_core_count;
    int num_procs, proxy_rem_cores, exec_rem_procs, global_active_processes, included_cores;
    int proxy_id, global_node_count, pcr, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /*
     * Find the process/core ratio that we can go to. The minimum is
     * zero (meaning there are no processes in the system). But if one
     * of the nodes is already oversubscribed, we take that as a hint
     * to mean that the other nodes can also be oversubscribed to the
     * same extent.
     */
    process_core_ratio = 0;
    global_node_count = 0;
    global_core_count = 0;
    global_active_processes = 0;
    for (node = node_list; node; node = node->next) {
        pcr = HYDU_dceil(node->active_processes, node->core_count);
        if (pcr > process_core_ratio)
            process_core_ratio = pcr;
        global_node_count++;
        global_core_count += node->core_count;
        global_active_processes += node->active_processes;
    }

    /* Create the list of proxies required to accommodate all the
     * processes. The proxy list follows these rules:
     *
     * 1. It will start at the first proxy that has a non-zero number
     * of available cores.
     *
     * 2. The maximum number of proxies cannot exceed the number of
     * nodes.
     *
     * 3. A proxy can never have zero processes assigned to it. The
     * below loop does not follow this rule; we make a second pass on
     * the list to enforce this rule.
     */
    pg->proxy_list = NULL;
    last_proxy = NULL;
    included_cores = 0;
    pcr = process_core_ratio;
    proxy_id = 0;
    for (node = node_list, i = 0; i < global_node_count; node = node->next) {
        if (node == NULL) {
            node = node_list;
            pcr++;
        }

        /* find the number of processes I can allocate on this proxy */
        c = (node->core_count * pcr - node->active_processes);
        if (c == 0)
            continue;

        /* create a proxy associated with this node */
        status = alloc_proxy(&proxy, pg, node);
        HYDU_ERR_POP(status, "error allocating proxy\n");

        proxy->proxy_id = proxy_id++;
        proxy->filler_processes = c;
        included_cores += c;

        if (pg->proxy_list == NULL)
            pg->proxy_list = proxy;
        else
            last_proxy->next = proxy;
        last_proxy = proxy;

        if (included_cores >= pg->pg_process_count)
            break;

        i++;
    }

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
        proxy = pg->proxy_list;

        pcr = process_core_ratio ? process_core_ratio : 1;

        exec_rem_procs = exec->proc_count;
        proxy_rem_cores = proxy->node->core_count * pcr - proxy->node->active_processes;

        while (exec) {
            num_procs = (exec_rem_procs > proxy_rem_cores) ? proxy_rem_cores : exec_rem_procs;

            exec_rem_procs -= num_procs;
            proxy_rem_cores -= num_procs;

            if (num_procs) {
                status = add_exec_to_proxy(exec, proxy, num_procs);
                HYDU_ERR_POP(status, "unable to add executable to proxy\n");
            }

            if (exec_rem_procs == 0) {
                exec = exec->next;
                if (exec)
                    exec_rem_procs = exec->proc_count;
                else
                    break;
            }

            if (proxy_rem_cores == 0) {
                proxy = proxy->next;

                if (proxy == NULL)
                    proxy = pg->proxy_list;

                if (proxy->node->node_id == 0)
                    pcr++;

                proxy_rem_cores =
                    proxy->node->core_count * pcr - proxy->node->active_processes -
                    proxy->proxy_process_count;
            }
        }
    }

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
        tmp[1] = HYDU_strdup("/");
        tmp[2] = HYDU_strdup(*wdir);
        tmp[3] = NULL;

        HYDU_FREE(*wdir);
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
