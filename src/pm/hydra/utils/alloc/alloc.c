/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

void HYDU_init_user_global(struct HYD_user_global *user_global)
{
    user_global->bootstrap = NULL;
    user_global->bootstrap_exec = NULL;

    user_global->binding = NULL;
    user_global->bindlib = NULL;

    user_global->ckpointlib = NULL;
    user_global->ckpoint_prefix = NULL;

    user_global->enablex = -1;
    user_global->debug = -1;

    HYDU_init_global_env(&user_global->global_env);
}

void HYDU_init_global_env(struct HYD_env_global *global_env)
{
    global_env->system = NULL;
    global_env->user = NULL;
    global_env->inherited = NULL;
    global_env->prop = NULL;
}

static void init_node(struct HYD_node *node)
{
    node->hostname = NULL;
    node->core_count = 0;
    node->next = NULL;
}

HYD_status HYDU_alloc_node(struct HYD_node **node)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*node, struct HYD_node *, sizeof(struct HYD_node), status);
    init_node(*node);

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
        HYDU_FREE(node);

        node = tnode;
    }
}

void HYDU_init_pg(struct HYD_pg *pg, int pgid)
{
    pg->pgid = pgid;
    pg->proxy_list = NULL;
    pg->pg_process_count = 0;
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

        if (pg->pg_scratch)
            HYDU_FREE(pg->pg_scratch);

        HYDU_FREE(pg);

        pg = tpg;
    }
}

HYD_status HYDU_alloc_proxy(struct HYD_proxy **proxy, struct HYD_pg *pg)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*proxy, struct HYD_proxy *, sizeof(struct HYD_proxy), status);

    init_node(&(*proxy)->node);

    (*proxy)->pg = pg;

    (*proxy)->proxy_id = -1;
    (*proxy)->exec_launch_info = NULL;

    (*proxy)->start_pid = -1;
    (*proxy)->proxy_process_count = 0;

    (*proxy)->exit_status = NULL;
    (*proxy)->control_fd = -1;

    (*proxy)->exec_list = NULL;
    (*proxy)->next = NULL;

    (*proxy)->proxy_scratch = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYDU_free_proxy_list(struct HYD_proxy *proxy_list)
{
    struct HYD_proxy *proxy, *tproxy;
    struct HYD_exec *exec, *texec;

    HYDU_FUNC_ENTER();

    proxy = proxy_list;
    while (proxy) {
        tproxy = proxy->next;

        if (proxy->node.hostname)
            HYDU_FREE(proxy->node.hostname);

        if (proxy->exec_launch_info) {
            HYDU_free_strlist(proxy->exec_launch_info);
            HYDU_FREE(proxy->exec_launch_info);
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

static HYD_status add_exec_to_proxy(struct HYD_exec *exec, struct HYD_proxy *proxy, int num_procs)
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
                                  struct HYD_pg *pg, int proc_offset)
{
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_node *node, *start_node;
    int proxy_rem_procs, exec_rem_procs, core_count, procs_left;
    int total_exec_procs, num_nodes, proxy_count, i, start_pid, offset;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    total_exec_procs = 0;
    for (exec = exec_list; exec; exec = exec->next)
        total_exec_procs += exec->proc_count;

    num_nodes = 0;
    core_count = 0;
    for (node = node_list; node; node = node->next) {
        num_nodes++;
        core_count += node->core_count;
    }

    if (total_exec_procs >= core_count)
        proxy_count = num_nodes;
    else {
        proxy_count = 0;
        procs_left = total_exec_procs;
        for (node = node_list; node; node = node->next) {
            proxy_count++;
            procs_left -= node->core_count;
            if (procs_left <= 0)
                break;
        }
    }

    /* First create the list of proxies we need */
    offset = proc_offset % core_count;
    for (node = node_list; node; node = node->next) {
        offset -= node->core_count;
        if (offset < 0)
            break;
    }
    start_node = node;

    if (offset + start_node->core_count) {
        /* we are starting on some offset within the node; the maximum
         * number of proxies can be larger than the total number of
         * nodes, since we might wrap around. */
        num_nodes++;
    }

    start_pid = 0;
    procs_left = total_exec_procs;
    for (i = 0, node = start_node; i < num_nodes; i++) {
        /* Make sure we need more proxies */
        if (i >= proxy_count)
            break;

        if (pg->proxy_list == NULL) {
            status = HYDU_alloc_proxy(&pg->proxy_list, pg);
            HYDU_ERR_POP(status, "unable to allocate proxy\n");
            proxy = pg->proxy_list;
        }
        else {
            status = HYDU_alloc_proxy(&proxy->next, pg);
            HYDU_ERR_POP(status, "unable to allocate proxy\n");
            proxy = proxy->next;
        }

        /* For the first node, use only the remaining cores. For the
         * last node, we need to make sure its not oversubscribed
         * since the first proxy we started on might repeat. */
        if (i == 0)
            proxy->node.core_count = -(offset); /* offset is negative */
        else if (i == (num_nodes - 1))
            proxy->node.core_count = node->core_count + offset;
        else
            proxy->node.core_count = node->core_count;

        proxy->proxy_id = i;
        proxy->start_pid = start_pid;
        proxy->node.hostname = HYDU_strdup(node->hostname);
        proxy->node.next = NULL;

        /* If we found enough proxies, break out */
        start_pid += proxy->node.core_count;
        procs_left -= proxy->node.core_count;
        if (procs_left <= 0)
            break;

        node = node->next;
        /* Handle the wrap around case for the nodes */
        if (node == NULL)
            node = node_list;
    }

    /* Now fill the proxies with the appropriate executable
     * information */
    proxy = pg->proxy_list;
    exec = exec_list;
    proxy_rem_procs = proxy->node.core_count;
    exec_rem_procs = exec ? exec->proc_count : 0;
    while (exec) {
        if (exec_rem_procs <= proxy_rem_procs) {
            status = add_exec_to_proxy(exec, proxy, exec_rem_procs);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");

            proxy_rem_procs -= exec_rem_procs;
            if (proxy_rem_procs == 0) {
                proxy = proxy->next;
                if (proxy == NULL)
                    proxy = pg->proxy_list;
                proxy_rem_procs = proxy->node.core_count;
            }

            exec = exec->next;
            exec_rem_procs = exec ? exec->proc_count : 0;
        }
        else {
            status = add_exec_to_proxy(exec, proxy, proxy_rem_procs);
            HYDU_ERR_POP(status, "unable to add executable to proxy\n");

            exec_rem_procs -= proxy_rem_procs;

            proxy = proxy->next;
            if (proxy == NULL)
                proxy = pg->proxy_list;
            proxy_rem_procs = proxy->node.core_count;
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
