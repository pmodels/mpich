/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmci.h"
#include "bsci.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

HYD_status HYD_pmcd_pmi_fill_in_proxy_args(char **proxy_args, char *control_port, int pgid)
{
    int i, arg, use_ddd, use_valgrind, enable_stdin;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    arg = 0;

    /* Hack to use ddd and valgrind with the proxy */
    if (MPL_env2bool("HYDRA_USE_DDD", &use_ddd) == 0)
        use_ddd = 0;
    if (MPL_env2bool("HYDRA_USE_VALGRIND", &use_valgrind) == 0)
        use_valgrind = 0;

    if (use_ddd)
        proxy_args[arg++] = HYDU_strdup("ddd");

    if (use_valgrind)
        proxy_args[arg++] = HYDU_strdup("valgrind");

    i = 0;
    path_str[i++] = HYDU_strdup(HYD_handle.base_path);
    path_str[i++] = HYDU_strdup("pmi_proxy");
    path_str[i] = NULL;
    status = HYDU_str_alloc_and_join(path_str, &proxy_args[arg++]);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(path_str);

    proxy_args[arg++] = HYDU_strdup("--control-port");
    proxy_args[arg++] = HYDU_strdup(control_port);

    if (HYD_handle.user_global.debug)
        proxy_args[arg++] = HYDU_strdup("--debug");

    proxy_args[arg++] = HYDU_strdup("--bootstrap");
    proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.bootstrap);

    if (HYD_handle.user_global.bootstrap_exec) {
        proxy_args[arg++] = HYDU_strdup("--bootstrap-exec");
        proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.bootstrap_exec);
    }

    proxy_args[arg++] = HYDU_strdup("--demux");
    proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.demux);

    if (HYD_handle.user_global.iface) {
        proxy_args[arg++] = HYDU_strdup("--iface");
        proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.iface);
    }

    proxy_args[arg++] = HYDU_strdup("--pgid");
    proxy_args[arg++] = HYDU_int_to_str(pgid);

    if (pgid == 0) {
        status = HYDT_dmx_stdin_valid(&enable_stdin);
        HYDU_ERR_POP(status, "unable to check if stdin is valid\n");
    }
    else {
        enable_stdin = 0;
    }

    proxy_args[arg++] = HYDU_strdup("--enable-stdin");
    proxy_args[arg++] = HYDU_int_to_str(enable_stdin);

    proxy_args[arg++] = HYDU_strdup("--proxy-id");
    proxy_args[arg++] = NULL;

    if (use_ddd) {
        HYDU_dump_noprefix(stdout, "\nUse proxy launch args: ");
        HYDU_print_strlist(proxy_args);
        HYDU_dump_noprefix(stdout, "\n");

        proxy_args[2] = NULL;
    }

    if (HYD_handle.user_global.debug) {
        HYDU_dump_noprefix(stdout, "\nProxy launch args: ");
        HYDU_print_strlist(proxy_args);
        HYDU_dump_noprefix(stdout, "\n");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_fill_in_exec_launch_info(char *pmi_port, int pmi_id, struct HYD_pg *pg)
{
    int i, arg, process_id;
    int inherited_env_count, user_env_count, system_env_count;
    int exec_count, total_args;
    static int proxy_count = 0;
    struct HYD_env *env;
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    char *mapping = NULL;
    HYD_status status = HYD_SUCCESS;

    status = HYD_pmcd_pmi_process_mapping(&mapping);
    HYDU_ERR_POP(status, "Unable to get process mapping information\n");

    /* Make sure the mapping is within the size allowed by PMI */
    if (strlen(mapping) > MAXVALLEN) {
        HYDU_FREE(mapping);
        mapping = NULL;
    }

    /* Create the arguments list for each proxy */
    process_id = 0;
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        for (inherited_env_count = 0, env = HYD_handle.user_global.global_env.inherited; env;
             env = env->next, inherited_env_count++);
        for (user_env_count = 0, env = HYD_handle.user_global.global_env.user; env;
             env = env->next, user_env_count++);
        for (system_env_count = 0, env = HYD_handle.user_global.global_env.system; env;
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

        proxy->exec_launch_info[arg++] = HYDU_strdup("--interface-env-name");
        proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.interface_env_name);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--hostname");
        proxy->exec_launch_info[arg++] = HYDU_strdup(proxy->node.hostname);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-core-count");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(HYD_handle.global_core_count);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-id");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(pmi_id);

        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-kvsname");
        proxy->exec_launch_info[arg++] = HYDU_strdup(pg_scratch->kvs->kvs_name);

        if (mapping) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-process-mapping");
            proxy->exec_launch_info[arg++] = HYDU_strdup(mapping);
        }

        if (proxy->node.local_binding) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--local-binding");
            proxy->exec_launch_info[arg++] = HYDU_strdup(proxy->node.local_binding);
        }

        if (HYD_handle.user_global.binding) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--binding");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.binding);
        }

        if (HYD_handle.user_global.bindlib) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--bindlib");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.bindlib);
        }

        if (HYD_handle.user_global.ckpointlib) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpointlib");
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.ckpointlib);
        }

        if (HYD_handle.user_global.ckpoint_prefix) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpoint-prefix");
            proxy->exec_launch_info[arg++] =
                HYDU_strdup(HYD_handle.user_global.ckpoint_prefix);
        }

        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-inherited-env");
        for (i = 0, env = HYD_handle.user_global.global_env.inherited; env;
             env = env->next, i++);
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

        for (env = HYD_handle.user_global.global_env.inherited; env; env = env->next) {
            status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
            HYDU_ERR_POP(status, "error converting env to string\n");
        }
        proxy->exec_launch_info[arg++] = NULL;

        arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-user-env");
        for (i = 0, env = HYD_handle.user_global.global_env.user; env; env = env->next, i++);
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

        for (env = HYD_handle.user_global.global_env.user; env; env = env->next) {
            status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
            HYDU_ERR_POP(status, "error converting env to string\n");
        }
        proxy->exec_launch_info[arg++] = NULL;

        arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
        proxy->exec_launch_info[arg++] = HYDU_strdup("--global-system-env");
        for (i = 0, env = HYD_handle.user_global.global_env.system; env; env = env->next, i++);
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(i);

        for (env = HYD_handle.user_global.global_env.system; env; env = env->next) {
            status = HYDU_env_to_str(env, &proxy->exec_launch_info[arg++]);
            HYDU_ERR_POP(status, "error converting env to string\n");
        }
        proxy->exec_launch_info[arg++] = NULL;

        arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
        if (HYD_handle.user_global.global_env.prop) {
            proxy->exec_launch_info[arg++] = HYDU_strdup("--genv-prop");
            proxy->exec_launch_info[arg++] =
                HYDU_strdup(HYD_handle.user_global.global_env.prop);
        }

        proxy->exec_launch_info[arg++] = HYDU_strdup("--start-pid");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(proxy->start_pid);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--proxy-core-count");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(proxy->node.core_count);
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

            process_id += exec->proc_count;
        }

        if (HYD_handle.user_global.debug) {
            HYDU_dump_noprefix(stdout, "Arguments being passed to proxy %d:\n", proxy_count++);
            HYDU_print_strlist(proxy->exec_launch_info);
            HYDU_dump_noprefix(stdout, "\n");
        }
    }

  fn_exit:
    if (mapping)
        HYDU_FREE(mapping);
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_allocate_kvs(struct HYD_pmcd_pmi_kvs **kvs, int pgid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*kvs, struct HYD_pmcd_pmi_kvs *, sizeof(struct HYD_pmcd_pmi_kvs), status);
    HYDU_snprintf((*kvs)->kvs_name, MAXNAMELEN, "kvs_%d_%d", (int) getpid(), pgid);
    (*kvs)->key_pair = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_free_pmi_kvs_list(struct HYD_pmcd_pmi_kvs *kvs_list)
{
    struct HYD_pmcd_pmi_kvs_pair *key_pair, *tmp;

    HYDU_FUNC_ENTER();

    key_pair = kvs_list->key_pair;
    while (key_pair) {
        tmp = key_pair->next;
        HYDU_FREE(key_pair);
        key_pair = tmp;
    }
    HYDU_FREE(kvs_list);

    HYDU_FUNC_EXIT();
}

static HYD_status init_pg_scratch(struct HYD_pg *pg)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    pg_scratch->barrier_count = 0;
    pg_scratch->control_listen_fd = -1;
    pg_scratch->pmi_listen_fd = -1;

    status = HYD_pmcd_pmi_allocate_kvs(&pg_scratch->kvs, pg->pgid);
    HYDU_ERR_POP(status, "unable to allocate kvs space\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_alloc_pg_scratch(struct HYD_pg *pg)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(pg->pg_scratch, void *, sizeof(struct HYD_pmcd_pmi_pg_scratch), status);

    status = init_pg_scratch(pg);
    HYDU_ERR_POP(status, "unable to create pg\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_pmi_free_pg_scratch(struct HYD_pg *pg)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;
    if (pg_scratch) {
        HYD_pmcd_free_pmi_kvs_list(pg_scratch->kvs);
        HYDU_FREE(pg_scratch);
        pg_scratch = NULL;
    }
}
