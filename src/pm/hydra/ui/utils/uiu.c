/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "uiu.h"

void HYD_uiu_init_params(void)
{
    HYDU_init_user_global(&HYD_handle.user_global);

    HYD_handle.ppn = -1;

    HYD_handle.base_path = NULL;

    HYD_handle.rmk = NULL;

    HYD_handle.port_range = NULL;
    HYD_handle.interface_env_name = NULL;

    HYD_handle.ckpoint_int = -1;

    HYD_handle.print_rank_map = -1;
    HYD_handle.print_all_exitcodes = -1;

    HYD_handle.ranks_per_proc = -1;

    HYD_handle.stdout_cb = NULL;
    HYD_handle.stderr_cb = NULL;

    HYD_handle.node_list = NULL;
    HYD_handle.global_core_count = 0;

    HYDU_init_pg(&HYD_handle.pg_list, 0);

    HYD_handle.pg_list.pgid = 0;
    HYD_handle.pg_list.next = NULL;

    HYD_handle.func_depth = 0;

#if defined ENABLE_PROFILING
    HYD_handle.enable_profiling = -1;
    HYD_handle.num_pmi_calls = 0;
#endif /* ENABLE_PROFILING */

    HYD_handle.append_rank = -1;
}

void HYD_uiu_free_params(void)
{
    if (HYD_handle.base_path)
        HYDU_FREE(HYD_handle.base_path);

    if (HYD_handle.rmk)
        HYDU_FREE(HYD_handle.rmk);

    if (HYD_handle.port_range)
        HYDU_FREE(HYD_handle.port_range);

    if (HYD_handle.interface_env_name)
        HYDU_FREE(HYD_handle.interface_env_name);

    if (HYD_handle.user_global.binding)
        HYDU_FREE(HYD_handle.user_global.binding);

    if (HYD_handle.user_global.bindlib)
        HYDU_FREE(HYD_handle.user_global.bindlib);

    if (HYD_handle.user_global.ckpointlib)
        HYDU_FREE(HYD_handle.user_global.ckpointlib);

    if (HYD_handle.user_global.ckpoint_prefix)
        HYDU_FREE(HYD_handle.user_global.ckpoint_prefix);

    if (HYD_handle.user_global.bootstrap)
        HYDU_FREE(HYD_handle.user_global.bootstrap);

    if (HYD_handle.user_global.demux)
        HYDU_FREE(HYD_handle.user_global.demux);

    if (HYD_handle.user_global.iface)
        HYDU_FREE(HYD_handle.user_global.iface);

    if (HYD_handle.user_global.bootstrap_exec)
        HYDU_FREE(HYD_handle.user_global.bootstrap_exec);

    if (HYD_handle.user_global.global_env.inherited)
        HYDU_env_free_list(HYD_handle.user_global.global_env.inherited);

    if (HYD_handle.user_global.global_env.system)
        HYDU_env_free_list(HYD_handle.user_global.global_env.system);

    if (HYD_handle.user_global.global_env.user)
        HYDU_env_free_list(HYD_handle.user_global.global_env.user);

    if (HYD_handle.user_global.global_env.prop)
        HYDU_FREE(HYD_handle.user_global.global_env.prop);

    if (HYD_handle.node_list)
        HYDU_free_node_list(HYD_handle.node_list);

    if (HYD_handle.pg_list.proxy_list)
        HYDU_free_proxy_list(HYD_handle.pg_list.proxy_list);

    if (HYD_handle.pg_list.next)
        HYDU_free_pg_list(HYD_handle.pg_list.next);

    /* Re-initialize everything to default values */
    HYD_uiu_init_params();
}

void HYD_uiu_print_params(void)
{
    struct HYD_env *env;
    struct HYD_proxy *proxy;
    struct HYD_exec *exec;
    int i;

    HYDU_FUNC_ENTER();

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "mpiexec options:\n");
    HYDU_dump_noprefix(stdout, "----------------\n");
    HYDU_dump_noprefix(stdout, "  Base path: %s\n", HYD_handle.base_path);
    HYDU_dump_noprefix(stdout, "  Bootstrap server: %s\n", HYD_handle.user_global.bootstrap);
    HYDU_dump_noprefix(stdout, "  Debug level: %d\n", HYD_handle.user_global.debug);
    HYDU_dump_noprefix(stdout, "  Enable X: %d\n", HYD_handle.user_global.enablex);

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "  Global environment:\n");
    HYDU_dump_noprefix(stdout, "  -------------------\n");
    for (env = HYD_handle.user_global.global_env.inherited; env; env = env->next)
        HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);

    if (HYD_handle.user_global.global_env.system) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  Hydra internal environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------------\n");
        for (env = HYD_handle.user_global.global_env.system; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    if (HYD_handle.user_global.global_env.user) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  User set environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------\n");
        for (env = HYD_handle.user_global.global_env.user; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_dump_noprefix(stdout, "    Proxy information:\n");
    HYDU_dump_noprefix(stdout, "    *********************\n");
    i = 1;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        HYDU_dump_noprefix(stdout, "      Proxy ID: %2d\n", i++);
        HYDU_dump_noprefix(stdout, "      -----------------\n");
        HYDU_dump_noprefix(stdout, "        Proxy name: %s\n", proxy->node.hostname);
        HYDU_dump_noprefix(stdout, "        Process count: %d\n", proxy->node.core_count);
        HYDU_dump_noprefix(stdout, "        Start PID: %d\n", proxy->start_pid);
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "        Proxy exec list:\n");
        HYDU_dump_noprefix(stdout, "        ....................\n");
        for (exec = proxy->exec_list; exec; exec = exec->next)
            HYDU_dump_noprefix(stdout, "          Exec: %s; Process count: %d\n",
                               exec->exec[0], exec->proc_count);
    }

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "=================================================");
    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_FUNC_EXIT();

    return;
}

HYD_status HYD_uiu_stdout_cb(void *buf, int buflen)
{
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_write(STDOUT_FILENO, buf, buflen, &sent, &closed);
    HYDU_ERR_POP(status, "unable to write data to stdout\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uiu_stderr_cb(void *buf, int buflen)
{
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_write(STDERR_FILENO, buf, buflen, &sent, &closed);
    HYDU_ERR_POP(status, "unable to write data to stdout\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
