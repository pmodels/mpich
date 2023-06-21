/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "utarray.h"

static UT_array *pg_list;

static void pg_dtor(void *_elt)
{
    struct HYD_pg *pg = _elt;

    if (pg->rankmap) {
        MPL_free(pg->rankmap);
    }

    if (pg->proxy_list)
        PMISERV_free_proxy_list(pg->proxy_list, pg->proxy_count);

    if (pg->user_node_list)
        HYDU_free_node_list(pg->user_node_list);

    if (pg->pg_scratch)
        HYD_pmcd_pmi_free_pg_scratch(pg);
}

void PMISERV_pg_init(void)
{
    static UT_icd pg_icd = { sizeof(struct HYD_pg), NULL, NULL, pg_dtor };
    utarray_new(pg_list, &pg_icd, MPL_MEM_OTHER);

    int pgid = PMISERV_pg_alloc();
    assert(pgid == 0);
}

int PMISERV_pg_alloc(void)
{
    HYDU_FUNC_ENTER();

    int pgid = utarray_len(pg_list);
    utarray_extend_back(pg_list, MPL_MEM_OTHER);
    struct HYD_pg *pg = (struct HYD_pg *) utarray_eltptr(pg_list, pgid);
    pg->pgid = pgid;
    pg->is_active = true;
    pg->spawner_pgid = -1;

    HYDU_FUNC_EXIT();
    return pgid;
}

void PMISERV_pg_finalize(void)
{
    utarray_free(pg_list);
}

int PMISERV_pg_max_id(void)
{
    return utarray_len(pg_list);
}

struct HYD_pg *PMISERV_pg_by_id(int pgid)
{
    if (pgid >= 0 && pgid < utarray_len(pg_list)) {
        return (struct HYD_pg *) utarray_eltptr(pg_list, pgid);
    } else {
        return NULL;
    }
}

/* -- proxy routines -- */

static void init_proxy(struct HYD_proxy *proxy, int pgid, struct HYD_node *node);
static HYD_status add_exec_to_proxy(struct HYD_exec *exec, struct HYD_proxy *proxy, int num_procs);

HYD_status PMISERV_create_proxy_list_singleton(struct HYD_node *node, int pgid,
                                               int *proxy_count_p, struct HYD_proxy **proxy_list_p)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_proxy *proxy;
    proxy = MPL_malloc(sizeof(struct HYD_proxy), MPL_MEM_OTHER);
    HYDU_ASSERT(proxy, status);

    init_proxy(proxy, pgid, node);

    proxy->proxy_id = 0;
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

HYD_status PMISERV_create_proxy_list(int count, struct HYD_exec *exec_list,
                                     struct HYD_node *node_list, int pgid, int *rankmap,
                                     int *proxy_count_p, struct HYD_proxy **proxy_list_p)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    /* NOTE: the nodes may have gaps */
    int max_node_id, min_node_id, num_nodes;
    max_node_id = min_node_id = rankmap[0];
    for (int i = 0; i < count; i++) {
        if (max_node_id < rankmap[i]) {
            max_node_id = rankmap[i];
        }
        if (min_node_id > rankmap[i]) {
            min_node_id = rankmap[i];
        }
    }
    num_nodes = max_node_id - min_node_id + 1;

    struct HYD_proxy *proxy_list = MPL_malloc(num_nodes * sizeof(struct HYD_proxy), MPL_MEM_OTHER);
    HYDU_ASSERT(proxy_list, status);

    for (struct HYD_node * node = node_list; node; node = node->next) {
        int id = node->node_id;
        if (id >= min_node_id && id <= max_node_id) {
            int i_proxy = id - min_node_id;
            init_proxy(&proxy_list[i_proxy], pgid, node);
        }
    }

    int rank = 0;

    struct HYD_exec *exec;
    int exec_rem_procs;
    exec = exec_list;
    exec_rem_procs = exec->proc_count;

    while (exec) {
        HYDU_ASSERT(exec_rem_procs, status);
        HYDU_ASSERT(rank < count, status);

        int node_id, c;
        node_id = rankmap[rank];
        c = 1;
        rank++;
        while (c < exec_rem_procs && rank < count && rankmap[rank] == node_id) {
            c++;
            rank++;
        }

        struct HYD_proxy *proxy = proxy_list + (node_id - min_node_id);
        status = add_exec_to_proxy(exec, proxy, c);
        HYDU_ERR_POP(status, "unable to add executable to proxy\n");

        exec_rem_procs -= c;
        if (exec_rem_procs == 0) {
            exec = exec->next;
            if (exec) {
                exec_rem_procs = exec->proc_count;
            } else {
                break;
            }
        }
    }

    /* transfer proxy_list to pg->proxy_list */
    int num_proxy;
    num_proxy = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (proxy_list[i].proxy_process_count > 0) {
            num_proxy++;
        }
    }

    *proxy_count_p = num_proxy;
    *proxy_list_p = MPL_malloc(num_proxy * sizeof(struct HYD_proxy), MPL_MEM_OTHER);
    HYDU_ASSERT(*proxy_list_p, status);

    int proxy_id = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (proxy_list[i].proxy_process_count > 0) {
            (*proxy_list_p)[proxy_id] = proxy_list[i];
            (*proxy_list_p)[proxy_id].proxy_id = proxy_id;
            proxy_id++;
        }
    }
    MPL_free(proxy_list);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void PMISERV_free_proxy_list(struct HYD_proxy *proxy_list, int count)
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

HYD_status PMISERV_proxy_list_to_host_list(struct HYD_proxy *proxy_list, int count,
                                           struct HYD_host **host_list)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_host *hosts;
    HYDU_MALLOC_OR_JUMP(hosts, struct HYD_host *, count * sizeof(struct HYD_host), status);

    for (int i = 0; i < count; i++) {
        hosts[i].hostname = proxy_list[i].node->hostname;
        hosts[i].user = proxy_list[i].node->user;
        hosts[i].core_count = proxy_list[i].node->core_count;
    }

    *host_list = hosts;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* static functions */

static void init_proxy(struct HYD_proxy *proxy, int pgid, struct HYD_node *node)
{
    proxy->node = node;
    proxy->pgid = pgid;

    proxy->proxy_id = -1;
    proxy->exec_launch_info = NULL;

    proxy->proxy_process_count = 0;
    proxy->filler_processes = 0;

    proxy->pid = NULL;
    proxy->exit_status = NULL;
    proxy->control_fd = HYD_FD_UNSET;

    proxy->exec_list = NULL;
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
