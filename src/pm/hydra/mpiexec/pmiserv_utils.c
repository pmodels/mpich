/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "pmci.h"
#include "bsci.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

static HYD_status gen_kvsname(char kvsname[], int pgid);

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
    if (!HYD_server_info.hybrid_hosts) {
        /* use abs path unless hybrid_hosts option is given */
        HYD_STRING_STASH(stash, MPL_strdup(HYD_server_info.base_path), status);
    }
    HYD_STRING_STASH(stash, MPL_strdup(HYDRA_PMI_PROXY), status);
    HYD_STRING_SPIT(stash, str, status);

    HYD_STRING_STASH(*proxy_stash, str, status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--control-port"), status);
    HYD_STRING_STASH(*proxy_stash, MPL_strdup(control_port), status);

    if (HYD_server_info.user_global.debug)
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--debug"), status);

    if (HYD_server_info.user_global.topo_debug)
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--topo-debug"), status);

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

    if (HYD_server_info.user_global.iface) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--iface"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYD_server_info.user_global.iface), status);
    }

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--pgid"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(pgid), status);

    ret = MPL_env2int("HYDRA_PROXY_RETRY_COUNT", &retries);
    if (ret == 0)
        retries = HYD_DEFAULT_RETRY_COUNT;

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--retries"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(retries), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--usize"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_server_info.user_global.usize), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--pmi-port"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_server_info.user_global.pmi_port), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--gpus-per-proc"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_server_info.user_global.gpus_per_proc),
                     status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--gpu-subdevs-per-proc"), status);
    HYD_STRING_STASH(*proxy_stash,
                     HYDU_int_to_str(HYD_server_info.user_global.gpu_subdevs_per_proc), status);

    if (pgid == 0 && HYD_server_info.is_singleton) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--singleton-port"), status);
        HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_server_info.singleton_port), status);

        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--singleton-pid"), status);
        HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_server_info.singleton_pid), status);
    }

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

static HYD_status add_env_to_exec_stash(struct HYD_string_stash *exec_stash, const char *optname,
                                        struct HYD_env *env_list)
{
    HYD_status status = HYD_SUCCESS;
    int i;
    struct HYD_env *env;

    HYD_STRING_STASH(*exec_stash, MPL_strdup(optname), status);
    for (i = 0, env = env_list; env; env = env->next, i++);
    HYD_STRING_STASH(*exec_stash, HYDU_int_to_str(i), status);

    for (env = env_list; env; env = env->next) {
        char *envstr;

        status = HYDU_env_to_str(env, &envstr);
        HYDU_ERR_POP(status, "error converting env to string\n");

        HYD_STRING_STASH(*exec_stash, envstr, status);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(struct HYD_pg *pg)
{
    int inherited_env_count, user_env_count, system_env_count, exec_count;
    int total_filler_processes, total_core_count;
    int pmi_id, *filler_pmi_ids = NULL, *nonfiller_pmi_ids = NULL;
    struct HYD_env *env;
    struct HYD_exec *exec;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *mapping = NULL, *map;
    struct HYD_string_stash stash, exec_stash;
    HYD_status status = HYD_SUCCESS;

    int mpl_err = MPL_rankmap_array_to_str(pg->rankmap, pg->pg_process_count, &mapping);
    HYDU_ASSERT(mpl_err == MPL_SUCCESS && mapping, status);

    /* Make sure the mapping is within the size allowed by PMI */
    if (strlen(mapping) > PMI_MAXVALLEN) {
        MPL_free(mapping);
        mapping = NULL;
    }

    /* Create the arguments list for each proxy */
    total_filler_processes = 0;
    for (int i = 0; i < pg->proxy_count; i++) {
        total_filler_processes += pg->proxy_list[i].filler_processes;
    }

    total_core_count = 0;
    for (int i = 0; i < pg->proxy_count; i++) {
        total_core_count += pg->proxy_list[i].node->core_count;
    }

    HYDU_MALLOC_OR_JUMP(filler_pmi_ids, int *, pg->proxy_count * sizeof(int), status);
    HYDU_MALLOC_OR_JUMP(nonfiller_pmi_ids, int *, pg->proxy_count * sizeof(int), status);

    pmi_id = 0;
    for (int i = 0; i < pg->proxy_count; i++) {
        filler_pmi_ids[i] = pmi_id;
        pmi_id += pg->proxy_list[i].filler_processes;
    }
    for (int i = 0; i < pg->proxy_count; i++) {
        nonfiller_pmi_ids[i] = pmi_id;
        pmi_id += pg->proxy_list[i].node->core_count;
    }

    for (int i = 0; i < pg->proxy_count; i++) {
        struct HYD_proxy *proxy = &pg->proxy_list[i];
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

        if (HYD_server_info.iface_ip_env_name) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--iface-ip-env-name"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(HYD_server_info.iface_ip_env_name), status);
        }

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
        HYD_STRING_STASH(stash, HYDU_int_to_str(filler_pmi_ids[i]), status);
        HYD_STRING_STASH(stash, MPL_strdup(","), status);
        HYD_STRING_STASH(stash, HYDU_int_to_str(nonfiller_pmi_ids[i]), status);
        HYD_STRING_SPIT(stash, map, status);

        HYD_STRING_STASH(exec_stash, map, status);

        HYD_STRING_STASH(exec_stash, MPL_strdup("--global-process-count"), status);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(pg->pg_process_count), status);

        HYD_STRING_STASH(exec_stash, MPL_strdup("--auto-cleanup"), status);
        HYD_STRING_STASH(exec_stash, HYDU_int_to_str(HYD_server_info.user_global.auto_cleanup),
                         status);

        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
        HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-kvsname"), status);
        HYD_STRING_STASH(exec_stash, MPL_strdup(pg_scratch->kvsname), status);

        if (pg->spawner_pgid != -1) {
            struct HYD_pg *spawner_pg = PMISERV_pg_by_id(pg->spawner_pgid);
            pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) spawner_pg->pg_scratch;
            HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-spawner-kvsname"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(pg_scratch->kvsname), status);
        }

        HYD_STRING_STASH(exec_stash, MPL_strdup("--pmi-process-mapping"), status);
        HYD_STRING_STASH(exec_stash, MPL_strdup(mapping), status);

        if (proxy->node->local_binding) {
            HYD_STRING_STASH(exec_stash, MPL_strdup("--binding"), status);
            HYD_STRING_STASH(exec_stash, MPL_strdup(proxy->node->local_binding), status);
        } else if (HYD_server_info.user_global.binding) {
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

        status = add_env_to_exec_stash(&exec_stash, "--global-inherited-env",
                                       HYD_server_info.user_global.global_env.inherited);
        HYDU_ERR_POP(status, "error adding global inherited env\n");

        status = add_env_to_exec_stash(&exec_stash, "--global-user-env",
                                       HYD_server_info.user_global.global_env.user);
        HYDU_ERR_POP(status, "error adding global user env\n");

        status = add_env_to_exec_stash(&exec_stash, "--global-system-env",
                                       HYD_server_info.user_global.global_env.system);
        HYDU_ERR_POP(status, "error adding global system env\n");

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

            status = add_env_to_exec_stash(&exec_stash, "--exec-local-env", exec->user_env);
            HYDU_ERR_POP(status, "error adding exec local env\n");

            if (exec->env_prop) {
                HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-env-prop"), status);
                HYD_STRING_STASH(exec_stash, MPL_strdup(exec->env_prop), status);
            }

            if (exec->wdir) {
                HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-wdir"), status);
                HYD_STRING_STASH(exec_stash, MPL_strdup(exec->wdir), status);
            }

            int argcnt;
            for (argcnt = 0; exec->exec[argcnt]; argcnt++);
            HYD_STRING_STASH(exec_stash, MPL_strdup("--exec-args"), status);
            HYD_STRING_STASH(exec_stash, HYDU_int_to_str(argcnt), status);

            for (int j = 0; exec->exec[j]; j++) {
                HYD_STRING_STASH(exec_stash, MPL_strdup(exec->exec[j]), status);
            }
        }

        if (HYD_server_info.user_global.debug) {
            HYDU_dump_noprefix(stdout, "Arguments being passed to proxy %d:\n", i);
            HYDU_print_strlist(exec_stash.strlist);
            HYDU_dump_noprefix(stdout, "\n");
        }

        status = HYDU_strdup_list(exec_stash.strlist, &proxy->exec_launch_info);
        HYDU_ERR_POP(status, "unable to dup strlist\n");

        HYD_STRING_STASH_FREE(exec_stash);
    }

  fn_exit:
    MPL_free(mapping);
    MPL_free(filler_pmi_ids);
    MPL_free(nonfiller_pmi_ids);
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_alloc_pg_scratch(struct HYD_pg *pg)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(pg->pg_scratch, void *, sizeof(struct HYD_pmcd_pmi_pg_scratch), status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    pg_scratch->barrier_count = 0;
    pg_scratch->epoch = 0;
    pg_scratch->fence_count = 0;

    pg_scratch->control_listen_fd = HYD_FD_UNSET;
    pg_scratch->pmi_listen_fd = HYD_FD_UNSET;

    pg_scratch->dead_processes = MPL_strdup("");
    pg_scratch->dead_process_count = 0;

    status = gen_kvsname(pg_scratch->kvsname, pg->pgid);
    HYDU_ERR_POP(status, "error in generating kvsname\n");

    status = HYD_pmcd_pmi_allocate_kvs(&pg_scratch->kvs);
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

        HYD_pmiserv_epoch_free(pg);
        MPL_free(pg_scratch->dead_processes);

        HYD_pmcd_free_pmi_kvs_list(pg_scratch->kvs);

        MPL_free(pg_scratch);
        pg->pg_scratch = NULL;
    }

    HYDU_FUNC_EXIT();
    return status;
}

static HYD_status gen_kvsname(char kvsname[], int pgid)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    char hostname[MAX_HOSTNAME_LEN - 40];       /* Remove space taken up by the integers and other
                                                 * characters below. */
    unsigned int seed;
    int rnd;

    if (gethostname(hostname, MAX_HOSTNAME_LEN - 40) < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");

    seed = (unsigned int) time(NULL);
    srand(seed);
    rnd = rand();

    MPL_snprintf_nowarn(kvsname, PMI_MAXKVSLEN, "kvs_%d_%d_%d_%s", (int) getpid(), pgid,
                        rnd, hostname);
  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}
