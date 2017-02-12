/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "pmci.h"
#include "bsci.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(struct HYD_string_stash *proxy_stash,
                                           char *control_port, int pgid)
{
    int use_ddd, use_valgrind, use_strace, retries, ret;
    char *str;
    struct HYD_string_stash stash;
    HYD_status status = HYD_SUCCESS;

    /* Hack to use ddd and valgrind with the proxy */
    if (MPL_env2bool("HYDRA_USE_DDD", &use_ddd) == 0)
        use_ddd = 0;
    if (MPL_env2bool("HYDRA_USE_VALGRIND", &use_valgrind) == 0)
        use_valgrind = 0;
    if (MPL_env2bool("HYDRA_USE_STRACE", &use_strace) == 0)
        use_strace = 0;

    HYD_STRING_STASH_INIT(*proxy_stash);
    if (use_ddd) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("ddd"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--args"), status);
    }

    if (use_valgrind) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("valgrind"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--leak-check=full"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--show-reachable=yes"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--track-origins=yes"), status);
    }

    if (use_strace) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("strace"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("-o"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("hydra_strace"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("-ff"), status);
    }

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup(HYD_server_info.base_path), status);
    HYD_STRING_STASH(stash, MPL_strdup(HYDRA_PMI_PROXY), status);
    HYD_STRING_SPIT(stash, str, status);

    HYD_STRING_STASH(*proxy_stash, str, status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--control-port"), status);
    HYD_STRING_STASH(*proxy_stash, MPL_strdup(control_port), status);

    if (HYD_server_info.user_global.debug)
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--debug"), status);

    if (HYDT_bsci_info.rmk) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--rmk"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYDT_bsci_info.rmk), status);
    }

    if (HYDT_bsci_info.launcher) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--launcher"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYDT_bsci_info.launcher), status);
    }

    if (HYDT_bsci_info.launcher_exec) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--launcher-exec"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYDT_bsci_info.launcher_exec), status);
    }

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--demux"), status);
    HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYD_server_info.user_global.demux), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--pgid"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(pgid), status);

    ret = MPL_env2int("HYDRA_PROXY_RETRY_COUNT", &retries);
    if (ret == 0)
        retries = HYD_DEFAULT_RETRY_COUNT;

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--retries"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(retries), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--usize"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_server_info.user_global.usize), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--proxy-id"), status);

    if (HYD_server_info.user_global.debug) {
        HYDU_dump_noprefix(stdout, "\nProxy launch args: ");
        HYDU_print_strlist(proxy_stash->strlist);
        HYDU_dump_noprefix(stdout, "\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_process_mapping(struct HYD_pg *pg, char **process_mapping_str)
{
    int is_equal, filler_round, core_count;
    struct HYD_string_stash stash;
    struct block {
        int start_idx;
        int num_nodes;
        int core_count;
        struct block *next;
    } *blocklist_head, *blocklist_tail = NULL, *block, *nblock;
    struct HYD_node *node;
    struct HYD_proxy *proxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /*
     * Blocks are of the format: (start node ID, number of nodes,
     * core count): (sid, nn, cc)
     *
     * Assume B1 and B2 are neighboring blocks. The following blocks
     * can be merged:
     *
     *   1. [B1(sid) + B1(nn) == B2(sid)] && [B1(cc) == B2(cc)]
     *
     *   2. [B1(sid) == B2(sid)] && [B1(nn) == 1]
     *
     * Special case: If all blocks are exactly the same, we delete all
     *               except one.
     */
    blocklist_head = NULL;

    filler_round = 1;
    for (proxy = pg->proxy_list;; proxy = proxy->next) {
        if (filler_round && proxy == NULL) {
            proxy = pg->proxy_list;
            filler_round = 0;
        }
        else if (proxy == NULL)
            break;

        if (filler_round && proxy->filler_processes == 0)
            continue;

        node = proxy->node;
        core_count = filler_round ? proxy->filler_processes : proxy->node->core_count;

        if (blocklist_head == NULL) {
            HYDU_MALLOC_OR_JUMP(block, struct block *, sizeof(struct block), status);
            block->start_idx = node->node_id;
            block->num_nodes = 1;
            block->core_count = core_count;
            block->next = NULL;

            blocklist_tail = blocklist_head = block;
        }
        else if (blocklist_tail->start_idx + blocklist_tail->num_nodes == node->node_id &&
                 blocklist_tail->core_count == core_count) {
            blocklist_tail->num_nodes++;
        }
        else if (blocklist_tail->start_idx == node->node_id && blocklist_tail->num_nodes == 1) {
            blocklist_tail->core_count += core_count;
        }
        else {
            HYDU_MALLOC_OR_JUMP(blocklist_tail->next, struct block *, sizeof(struct block), status);
            blocklist_tail = blocklist_tail->next;
            blocklist_tail->start_idx = node->node_id;
            blocklist_tail->num_nodes = 1;
            blocklist_tail->core_count = core_count;
            blocklist_tail->next = NULL;
        }
    }

    /* If all the blocks are equivalent, just use one block */
    is_equal = 1;
    for (block = blocklist_head; block->next; block = block->next) {
        if (block->start_idx != block->next->start_idx ||
            block->core_count != block->next->core_count) {
            is_equal = 0;
            break;
        }
    }
    if (is_equal) {
        for (block = blocklist_head; block->next;) {
            nblock = block->next;
            block->next = nblock->next;
            MPL_free(nblock);
        }
        blocklist_tail = blocklist_head;
    }

    /* Create the mapping out of the blocks */
    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup("("), status);
    HYD_STRING_STASH(stash, MPL_strdup("vector,"), status);
    for (block = blocklist_head; block; block = block->next) {
        HYD_STRING_STASH(stash, MPL_strdup("("), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(block->start_idx), status);
        HYD_STRING_STASH(stash, MPL_strdup(","), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(block->num_nodes), status);
        HYD_STRING_STASH(stash, MPL_strdup(","), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(block->core_count), status);
        HYD_STRING_STASH(stash, MPL_strdup(")"), status);
        if (block->next)
            HYD_STRING_STASH(stash, MPL_strdup(","), status);
    }
    HYD_STRING_STASH(stash, MPL_strdup(")"), status);

    HYD_STRING_SPIT(stash, *process_mapping_str, status);

    for (block = blocklist_head; block; block = nblock) {
        nblock = block->next;
        MPL_free(block);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(struct HYD_pg *pg)
{
    int i, inherited_env_count, user_env_count, system_env_count, exec_count;
    int proxy_count, total_filler_processes, total_core_count;
    int pmi_id, *filler_pmi_ids = NULL, *nonfiller_pmi_ids = NULL;
    struct HYD_env *env;
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_node *node;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *mapping = NULL, *map;
    struct HYD_string_stash stash, exec_stash;
    HYD_status status = HYD_SUCCESS;

    status = pmi_process_mapping(pg, &mapping);
    HYDU_ERR_POP(status, "Unable to get process mapping information\n");
    HYDU_ASSERT(mapping, status);

    /* Make sure the mapping is within the size allowed by PMI */
    if (strlen(mapping) > PMI_MAXVALLEN) {
        MPL_free(mapping);
        mapping = NULL;
    }

    /* Create the arguments list for each proxy */
    total_filler_processes = 0;
    proxy_count = 0;
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        total_filler_processes += proxy->filler_processes;
        proxy_count++;
    }

    total_core_count = 0;
    for (node = HYD_server_info.node_list; node; node = node->next)
        total_core_count += node->core_count;

    HYDU_MALLOC_OR_JUMP(filler_pmi_ids, int *, proxy_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(nonfiller_pmi_ids, int *, proxy_count * sizeof(int), status);

    pmi_id = 0;
    for (proxy = pg->proxy_list, i = 0; proxy; proxy = proxy->next, i++) {
        filler_pmi_ids[i] = pmi_id;
        pmi_id += proxy->filler_processes;
    }
    for (proxy = pg->proxy_list, i = 0; proxy; proxy = proxy->next, i++) {
        nonfiller_pmi_ids[i] = pmi_id;
        pmi_id += proxy->node->core_count;
    }

    proxy_count = 0;
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        for (inherited_env_count = 0, env = HYD_server_info.user_global.global_env.inherited;
             env; env = env->next, inherited_env_count++);
        for (user_env_count = 0, env = HYD_server_info.user_global.global_env.user; env;
             env = env->next, user_env_count++);
        for (system_env_count = 0, env = HYD_server_info.user_global.global_env.system; env;
             env = env->next, system_env_count++);

        for (exec_count = 0, exec = proxy->exec_list; exec; exec = exec->next)
            exec_count++;

        HYD_STRING_STASH_INIT(exec_stash);

        HYD_STRING_STASH(exec_stash, MPL_strdup("--version"), status);
        HYD_STRING_STASH(exec_stash, MPL_strdup(HYDRA_VERSION), status);

        HYD_STRING_STASH(exec_stash, MPL_strdup("--hostname"), status);
        HYD_STRING_STASH(exec_stash, MPL_strdup(proxy->node->hostname), status);

        /* This map has three fields: filler cores on this node,
         * remaining cores on this node, total cores in the system */
        HYD_STRING_STASH(exec_stash, MPL_strdup("--global-core-map"), status);

        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_int_to_str(proxy->filler_processes), status);
        HYD_STRING_STASH(stash, MPL_strdup(","), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(proxy->node->core_count), status);
        HYD_STRING_STASH(stash, MPL_strdup(","), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(total_core_count), status);
        HYD_STRING_SPIT(stash, map, status);

        HYD_STRING_STASH(exec_stash, map, status);

        /* This map has two fields: start PMI ID during the filler
         * phase, start PMI ID for the remaining phase */
        HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-id-map"), status);

        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_int_to_str(filler_pmi_ids[proxy_count]), status);
        HYD_STRING_STASH(stash, MPL_strdup(","), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(nonfiller_pmi_ids[proxy_count]), status);
        HYD_STRING_SPIT(stash, map, status);

        HYD_STRING_STASH(exec_stash, map, status);

        HYD_STRING_STASH(exec_stash, MPL_strdup("--global-process-count"), status);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(pg->pg_process_count), status);

        HYD_STRING_STASH(exec_stash, MPL_strdup("--auto-cleanup"), status);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(HYD_server_info.user_global.auto_cleanup),
                         status);

        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
        HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-kvsname"), status);
        HYD_STRING_STASH(exec_stash, MPL_strdup(pg_scratch->kvs->kvsname), status);

        if (pg->spawner_pg) {
            pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->spawner_pg->pg_scratch;
            HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-spawner-kvsname"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(pg_scratch->kvs->kvsname), status);
        }

        HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-process-mapping"), status);
        HYD_STRING_STASH(exec_stash, MPL_strdup(mapping), status);

        if (proxy->node->local_binding) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--binding"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(proxy->node->local_binding), status);
        }
        else if (HYD_server_info.user_global.binding) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--binding"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(HYD_server_info.user_global.binding), status);
        }

        if (HYD_server_info.user_global.mapping) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--mapping"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(HYD_server_info.user_global.mapping), status);
        }

        if (HYD_server_info.user_global.membind) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--membind"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(HYD_server_info.user_global.membind), status);
        }

        if (HYD_server_info.user_global.topolib) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--topolib"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(HYD_server_info.user_global.topolib), status);
        }

        HYD_STRING_STASH(exec_stash, MPL_strdup("--global-inherited-env"), status);
        for (i = 0, env = HYD_server_info.user_global.global_env.inherited; env;
             env = env->next, i++);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(i), status);

        for (env = HYD_server_info.user_global.global_env.inherited; env; env = env->next) {
            char *envstr;

            status = HYDU_env_to_str(env, &envstr);
            HYDU_ERR_POP(status, "error converting env to string\n");

            HYD_STRING_STASH(exec_stash, envstr, status);
        }

        HYD_STRING_STASH(exec_stash, MPL_strdup("--global-user-env"), status);
        for (i = 0, env = HYD_server_info.user_global.global_env.user; env; env = env->next, i++);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(i), status);

        for (env = HYD_server_info.user_global.global_env.user; env; env = env->next) {
            char *envstr;

            status = HYDU_env_to_str(env, &envstr);
            HYDU_ERR_POP(status, "error converting env to string\n");

            HYD_STRING_STASH(exec_stash, envstr, status);
        }

        HYD_STRING_STASH(exec_stash, MPL_strdup("--global-system-env"), status);
        for (i = 0, env = HYD_server_info.user_global.global_env.system; env; env = env->next, i++);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(i), status);

        for (env = HYD_server_info.user_global.global_env.system; env; env = env->next) {
            char *envstr;

            status = HYDU_env_to_str(env, &envstr);
            HYDU_ERR_POP(status, "error converting env to string\n");

            HYD_STRING_STASH(exec_stash, envstr, status);
        }

        if (HYD_server_info.user_global.global_env.prop) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--genv-prop"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(HYD_server_info.user_global.global_env.prop),
                             status);
        }

        HYD_STRING_STASH(exec_stash, MPL_strdup("--proxy-core-count"), status);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(proxy->node->core_count), status);

        /* Now pass the local executable information */
        for (exec = proxy->exec_list; exec; exec = exec->next) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--exec"), status);

            HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-appnum"), status);
            HYD_STRING_STASH(exec_stash, HYDU_int_to_str(exec->appnum), status);

            HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-proc-count"), status);
            HYD_STRING_STASH(exec_stash, HYDU_int_to_str(exec->proc_count), status);

            HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-local-env"), status);
            for (i = 0, env = exec->user_env; env; env = env->next, i++);
            HYD_STRING_STASH(exec_stash, HYDU_int_to_str(i), status);

            for (env = exec->user_env; env; env = env->next) {
                char *envstr;

                status = HYDU_env_to_str(env, &envstr);
                HYDU_ERR_POP(status, "error converting env to string\n");

                HYD_STRING_STASH(exec_stash, envstr, status);
            }

            if (exec->env_prop) {
                HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-env-prop"), status);
                HYD_STRING_STASH(exec_stash, MPL_strdup(exec->env_prop), status);
            }

            if (exec->wdir) {
                HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-wdir"), status);
                HYD_STRING_STASH(exec_stash, MPL_strdup(exec->wdir), status);
            }

            HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-args"), status);
            for (i = 0; exec->exec[i]; i++);
            HYD_STRING_STASH(exec_stash, HYDU_int_to_str(i), status);

            for (i = 0; exec->exec[i]; i++)
                HYD_STRING_STASH(exec_stash, MPL_strdup(exec->exec[i]), status);
        }

        if (HYD_server_info.user_global.debug) {
            HYDU_dump_noprefix(stdout, "Arguments being passed to proxy %d:\n", proxy_count);
            HYDU_print_strlist(exec_stash.strlist);
            HYDU_dump_noprefix(stdout, "\n");
        }

        status = HYDU_strdup_list(exec_stash.strlist, &proxy->exec_launch_info);
        HYDU_ERR_POP(status, "unable to dup strlist\n");

        HYD_STRING_STASH_FREE(exec_stash);

        proxy_count++;
    }

  fn_exit:
    if (mapping)
        MPL_free(mapping);
    MPL_free(filler_pmi_ids);
    MPL_free(nonfiller_pmi_ids);
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_alloc_pg_scratch(struct HYD_pg *pg)
{
    int i;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(pg->pg_scratch, void *, sizeof(struct HYD_pmcd_pmi_pg_scratch), status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    pg_scratch->barrier_count = 0;

    HYDU_MALLOC_OR_JUMP(pg_scratch->ecount, struct HYD_pmcd_pmi_ecount *,
                        pg->pg_process_count * sizeof(struct HYD_pmcd_pmi_ecount), status);
    for (i = 0; i < pg->pg_process_count; i++) {
        pg_scratch->ecount[i].fd = HYD_FD_UNSET;
        pg_scratch->ecount[i].pid = -1;
        pg_scratch->ecount[i].epoch = -1;
    }

    pg_scratch->control_listen_fd = HYD_FD_UNSET;
    pg_scratch->pmi_listen_fd = HYD_FD_UNSET;

    pg_scratch->dead_processes = MPL_strdup("");
    pg_scratch->dead_process_count = 0;

    status = HYD_pmcd_pmi_allocate_kvs(&pg_scratch->kvs, pg->pgid);
    HYDU_ERR_POP(status, "unable to allocate kvs space\n");

    pg_scratch->keyval_dist_count = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_free_pg_scratch(struct HYD_pg *pg)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pg->pg_scratch) {
        pg_scratch = pg->pg_scratch;

        if (pg_scratch->ecount)
            MPL_free(pg_scratch->ecount);

        if (pg_scratch->dead_processes)
            MPL_free(pg_scratch->dead_processes);

        HYD_pmcd_free_pmi_kvs_list(pg_scratch->kvs);

        MPL_free(pg_scratch);
        pg->pg_scratch = NULL;
    }

    HYDU_FUNC_EXIT();
    return status;
}
