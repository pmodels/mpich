/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "pmci.h"
#include "bsci.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(char **proxy_args, char *control_port, int pgid)
{
    int i, arg, use_ddd, use_valgrind, use_strace, retries, ret;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    arg = 0;

    /* Hack to use ddd and valgrind with the proxy */
    if (MPL_env2bool("HYDRA_USE_DDD", &use_ddd) == 0)
        use_ddd = 0;
    if (MPL_env2bool("HYDRA_USE_VALGRIND", &use_valgrind) == 0)
        use_valgrind = 0;
    if (MPL_env2bool("HYDRA_USE_STRACE", &use_strace) == 0)
        use_strace = 0;

    if (use_ddd) {
        proxy_args[arg++] = HYDU_strdup("ddd");
        proxy_args[arg++] = HYDU_strdup("--args");
    }

    if (use_valgrind) {
        proxy_args[arg++] = HYDU_strdup("valgrind");
        proxy_args[arg++] = HYDU_strdup("--leak-check=full");
        proxy_args[arg++] = HYDU_strdup("--show-reachable=yes");
        proxy_args[arg++] = HYDU_strdup("--track-origins=yes");
    }

    if (use_strace) {
        proxy_args[arg++] = HYDU_strdup("strace");
        proxy_args[arg++] = HYDU_strdup("-o");
        proxy_args[arg++] = HYDU_strdup("hydra_strace");
        proxy_args[arg++] = HYDU_strdup("-ff");
    }

    i = 0;
    path_str[i++] = HYDU_strdup(HYD_server_info.base_path);
    path_str[i++] = HYDU_strdup("hydra_pmi_proxy");
    path_str[i] = NULL;
    status = HYDU_str_alloc_and_join(path_str, &proxy_args[arg++]);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(path_str);

    proxy_args[arg++] = HYDU_strdup("--control-port");
    proxy_args[arg++] = HYDU_strdup(control_port);

    if (HYD_server_info.user_global.debug)
        proxy_args[arg++] = HYDU_strdup("--debug");

    if (HYDT_bsci_info.rmk) {
        proxy_args[arg++] = HYDU_strdup("--rmk");
        proxy_args[arg++] = HYDU_strdup(HYDT_bsci_info.rmk);
    }

    if (HYDT_bsci_info.launcher) {
        proxy_args[arg++] = HYDU_strdup("--launcher");
        proxy_args[arg++] = HYDU_strdup(HYDT_bsci_info.launcher);
    }

    if (HYDT_bsci_info.launcher_exec) {
        proxy_args[arg++] = HYDU_strdup("--launcher-exec");
        proxy_args[arg++] = HYDU_strdup(HYDT_bsci_info.launcher_exec);
    }

    proxy_args[arg++] = HYDU_strdup("--demux");
    proxy_args[arg++] = HYDU_strdup(HYD_server_info.user_global.demux);

    if (HYD_server_info.user_global.iface) {
        proxy_args[arg++] = HYDU_strdup("--iface");
        proxy_args[arg++] = HYDU_strdup(HYD_server_info.user_global.iface);
    }

    proxy_args[arg++] = HYDU_strdup("--pgid");
    proxy_args[arg++] = HYDU_int_to_str(pgid);

    ret = MPL_env2int("HYDRA_PROXY_RETRY_COUNT", &retries);
    if (ret == 0)
        retries = HYD_DEFAULT_RETRY_COUNT;

    proxy_args[arg++] = HYDU_strdup("--retries");
    proxy_args[arg++] = HYDU_int_to_str(retries);

    proxy_args[arg++] = HYDU_strdup("--usize");
    proxy_args[arg++] = HYDU_int_to_str(HYD_server_info.user_global.usize);

    proxy_args[arg++] = HYDU_strdup("--proxy-id");
    proxy_args[arg++] = NULL;

    if (HYD_server_info.user_global.debug) {
        HYDU_dump_noprefix(stdout, "\nProxy launch args: ");
        HYDU_print_strlist(proxy_args);
        HYDU_dump_noprefix(stdout, "\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_process_mapping(struct HYD_pg *pg, char **process_mapping_str)
{
    int i, is_equal, filler_round, core_count;
    char *tmp[HYD_NUM_TMP_STRINGS];
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
            HYDU_MALLOC(block, struct block *, sizeof(struct block), status);
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
            HYDU_MALLOC(blocklist_tail->next, struct block *, sizeof(struct block), status);
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
            HYDU_FREE(nblock);
        }
        blocklist_tail = blocklist_head;
    }

  create_mapping_key:
    /* Create the mapping out of the blocks */
    i = 0;
    tmp[i++] = HYDU_strdup("(");
    tmp[i++] = HYDU_strdup("vector,");
    for (block = blocklist_head; block; block = block->next) {
        tmp[i++] = HYDU_strdup("(");
        tmp[i++] = HYDU_int_to_str(block->start_idx);
        tmp[i++] = HYDU_strdup(",");
        tmp[i++] = HYDU_int_to_str(block->num_nodes);
        tmp[i++] = HYDU_strdup(",");
        tmp[i++] = HYDU_int_to_str(block->core_count);
        tmp[i++] = HYDU_strdup(")");
        if (block->next)
            tmp[i++] = HYDU_strdup(",");
        HYDU_STRLIST_CONSOLIDATE(tmp, i, status);
    }
    tmp[i++] = HYDU_strdup(")");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, process_mapping_str);
    HYDU_ERR_POP(status, "error while joining strings\n");

    HYDU_free_strlist(tmp);

    for (block = blocklist_head; block; block = nblock) {
        nblock = block->next;
        HYDU_FREE(block);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(struct HYD_pg *pg)
{
    int i, arg, inherited_env_count, user_env_count, system_env_count, exec_count;
    int total_args, proxy_count, total_filler_processes, total_core_count;
    int pmi_id, *filler_pmi_ids = NULL, *nonfiller_pmi_ids = NULL;
    struct HYD_env *env;
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_node *node;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *mapping = NULL, *map = NULL;
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    status = pmi_process_mapping(pg, &mapping);
    HYDU_ERR_POP(status, "Unable to get process mapping information\n");
    HYDU_ASSERT(mapping, status);

    /* Make sure the mapping is within the size allowed by PMI */
    if (strlen(mapping) > PMI_MAXVALLEN) {
        HYDU_FREE(mapping);
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

    HYDU_MALLOC(filler_pmi_ids, int *, proxy_count * sizeof(int), status);
    HYDU_MALLOC(nonfiller_pmi_ids, int *, proxy_count * sizeof(int), status);

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

        total_args = HYD_NUM_TMP_STRINGS;       /* For the basic arguments */

        /* Environments */
        total_args += inherited_env_count;
        total_args += user_env_count;
        total_args += system_env_count;

        /* For each exec add a few strings */
        total_args += (exec_count * HYD_NUM_TMP_STRINGS);

        /* Add a few strings for the remaining arguments */
        total_args += HYD_NUM_TMP_STRINGS;

        HYDU_MALLOC(proxy->exec_launch_info, char **, total_args * sizeof(char *), status);

        arg = 0;
        proxy->exec_launch_info[arg++] = HYDU_strdup("--version");
        proxy->exec_launch_info[arg++] = HYDU_strdup(HYDRA_VERSION);

        if (HYD_server_info.iface_ip_env_name) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--iface-ip-env-name");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_server_info.iface_ip_env_name);
        }

        proxy->exec_launch_info[arg++] = HYDU_strdup("--hostname");
        proxy->exec_launch_info[arg++] = HYDU_strdup(proxy->node->hostname);

        /* This map has three fields: filler cores on this node,
         * remaining cores on this node, total cores in the system */
        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-core-map");
        tmp[0] = HYDU_int_to_str(proxy->filler_processes);
        tmp[1] = HYDU_strdup(",");
        tmp[2] = HYDU_int_to_str(proxy->node->core_count);
        tmp[3] = HYDU_strdup(",");
        tmp[4] = HYDU_int_to_str(total_core_count);
        tmp[5] = NULL;
        status = HYDU_str_alloc_and_join(tmp, &map);
        HYDU_ERR_POP(status, "unable to join strings\n");
        proxy->exec_launch_info[arg++] = map;
        HYDU_free_strlist(tmp);

        /* This map has two fields: start PMI ID during the filler
         * phase, start PMI ID for the remaining phase */
        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-id-map");
        tmp[0] = HYDU_int_to_str(filler_pmi_ids[proxy_count]);
        tmp[1] = HYDU_strdup(",");
        tmp[2] = HYDU_int_to_str(nonfiller_pmi_ids[proxy_count]);
        tmp[3] = NULL;
        status = HYDU_str_alloc_and_join(tmp, &map);
        HYDU_ERR_POP(status, "unable to join strings\n");
        proxy->exec_launch_info[arg++] = map;
        HYDU_free_strlist(tmp);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-process-count");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(pg->pg_process_count);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--auto-cleanup");
        proxy->exec_launch_info[arg++] =
            HYDU_int_to_str(HYD_server_info.user_global.auto_cleanup);

        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-kvsname");
        proxy->exec_launch_info[arg++] = HYDU_strdup(pg_scratch->kvs->kvsname);

        if (pg->spawner_pg) {
            pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->spawner_pg->pg_scratch;
            proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-spawner-kvsname");
            proxy->exec_launch_info[arg++] = HYDU_strdup(pg_scratch->kvs->kvsname);
        }

        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-process-mapping");
        proxy->exec_launch_info[arg++] = HYDU_strdup(mapping);

        if (proxy->node->local_binding) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--binding");
            proxy->exec_launch_info[arg++] = HYDU_strdup(proxy->node->local_binding);
        }
        else if (HYD_server_info.user_global.binding) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--binding");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_server_info.user_global.binding);
        }

        if (HYD_server_info.user_global.mapping) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--mapping");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_server_info.user_global.mapping);
        }

        if (HYD_server_info.user_global.membind) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--membind");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_server_info.user_global.membind);
        }

        if (HYD_server_info.user_global.topolib) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--topolib");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_server_info.user_global.topolib);
        }

        if (HYD_server_info.user_global.ckpointlib) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpointlib");
            proxy->exec_launch_info[arg++] =
                HYDU_strdup(HYD_server_info.user_global.ckpointlib);
        }

        if (HYD_server_info.user_global.ckpoint_prefix) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpoint-prefix");
            proxy->exec_launch_info[arg++] =
                HYDU_strdup(HYD_server_info.user_global.ckpoint_prefix);
        }

        if (HYD_server_info.user_global.ckpoint_num) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpoint-num");
            proxy->exec_launch_info[arg++] =
                HYDU_int_to_str(HYD_server_info.user_global.ckpoint_num);
        }

        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-inherited-env");
        for (i = 0, env = HYD_server_info.user_global.global_env.inherited; env;
             env = env->next, i++);
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

        for (env = HYD_server_info.user_global.global_env.inherited; env; env = env->next) {
            status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
            HYDU_ERR_POP(status, "error converting env to string\n");
        }
        proxy->exec_launch_info[arg++] = NULL;

        arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-user-env");
        for (i = 0, env = HYD_server_info.user_global.global_env.user; env;
             env = env->next, i++);
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

        for (env = HYD_server_info.user_global.global_env.user; env; env = env->next) {
            status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
            HYDU_ERR_POP(status, "error converting env to string\n");
        }
        proxy->exec_launch_info[arg++] = NULL;

        arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-system-env");
        for (i = 0, env = HYD_server_info.user_global.global_env.system; env;
             env = env->next, i++);
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

        for (env = HYD_server_info.user_global.global_env.system; env; env = env->next) {
            status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
            HYDU_ERR_POP(status, "error converting env to string\n");
        }
        proxy->exec_launch_info[arg++] = NULL;

        arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
        if (HYD_server_info.user_global.global_env.prop) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--genv-prop");
            proxy->exec_launch_info[arg++] =
                HYDU_strdup(HYD_server_info.user_global.global_env.prop);
        }

        proxy->exec_launch_info[arg++] = HYDU_strdup("--proxy-core-count");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(proxy->node->core_count);
        proxy->exec_launch_info[arg++] = NULL;

        /* Now pass the local executable information */
        for (exec = proxy->exec_list; exec; exec = exec->next) {
            arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec");

            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-appnum");
            proxy->exec_launch_info[arg++] = HYDU_int_to_str(exec->appnum);

            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-proc-count");
            proxy->exec_launch_info[arg++] = HYDU_int_to_str(exec->proc_count);

            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-local-env");
            for (i = 0, env = exec->user_env; env; env = env->next, i++);
            proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

            for (env = exec->user_env; env; env = env->next) {
                status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
                HYDU_ERR_POP(status, "error converting env to string\n");
            }
            proxy->exec_launch_info[arg++] = NULL;

            if (exec->env_prop) {
                arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
                proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-env-prop");
                proxy->exec_launch_info[arg++] = HYDU_strdup(exec->env_prop);
                proxy->exec_launch_info[arg++] = NULL;
            }

            arg = HYDU_strlist_lastidx(proxy->exec_launch_info);

            if (exec->wdir) {
                proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-wdir");
                proxy->exec_launch_info[arg++] = HYDU_strdup(exec->wdir);
            }

            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-args");
            for (i = 0; exec->exec[i]; i++);
            proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);
            proxy->exec_launch_info[arg++] = NULL;

            HYDU_list_append_strlist(exec->exec, proxy->exec_launch_info);
        }

        if (HYD_server_info.user_global.debug) {
            HYDU_dump_noprefix(stdout, "Arguments being passed to proxy %d:\n", proxy_count);
            HYDU_print_strlist(proxy->exec_launch_info);
            HYDU_dump_noprefix(stdout, "\n");
        }

        proxy_count++;
    }

  fn_exit:
    if (mapping)
        HYDU_FREE(mapping);
    HYDU_FREE(filler_pmi_ids);
    HYDU_FREE(nonfiller_pmi_ids);
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

    HYDU_MALLOC(pg->pg_scratch, void *, sizeof(struct HYD_pmcd_pmi_pg_scratch), status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    pg_scratch->barrier_count = 0;

    HYDU_MALLOC(pg_scratch->ecount, struct HYD_pmcd_pmi_ecount *,
                pg->pg_process_count * sizeof(struct HYD_pmcd_pmi_ecount), status);
    for (i = 0; i < pg->pg_process_count; i++) {
        pg_scratch->ecount[i].fd = HYD_FD_UNSET;
        pg_scratch->ecount[i].pid = -1;
        pg_scratch->ecount[i].epoch = -1;
    }

    pg_scratch->control_listen_fd = HYD_FD_UNSET;
    pg_scratch->pmi_listen_fd = HYD_FD_UNSET;

    pg_scratch->dead_processes = HYDU_strdup("");
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
            HYDU_FREE(pg_scratch->ecount);

        if (pg_scratch->dead_processes)
            HYDU_FREE(pg_scratch->dead_processes);

        HYD_pmcd_free_pmi_kvs_list(pg_scratch->kvs);

        HYDU_FREE(pg_scratch);
        pg->pg_scratch = NULL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
