/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "ui.h"
#include "uiu.h"

void HYD_uiu_init_params(void)
{
    HYDU_init_user_global(&HYD_server_info.user_global);

    HYD_server_info.base_path = NULL;

    HYD_server_info.port_range = NULL;
    HYD_server_info.interface_env_name = NULL;

    HYD_server_info.nameserver = NULL;
    HYD_server_info.local_hostname = NULL;

    HYD_server_info.stdout_cb = NULL;
    HYD_server_info.stderr_cb = NULL;

    HYD_server_info.node_list = NULL;
    HYD_server_info.global_core_count = 0;

    HYDU_init_pg(&HYD_server_info.pg_list, 0);

    HYD_server_info.pg_list.pgid = 0;
    HYD_server_info.pg_list.next = NULL;

#if defined ENABLE_PROFILING
    HYD_server_info.enable_profiling = -1;
    HYD_server_info.num_pmi_calls = 0;
#endif /* ENABLE_PROFILING */

    HYD_ui_info.prepend_rank = -1;
}

void HYD_uiu_free_params(void)
{
    HYDU_finalize_user_global(&HYD_server_info.user_global);

    if (HYD_server_info.base_path)
        HYDU_FREE(HYD_server_info.base_path);

    if (HYD_server_info.port_range)
        HYDU_FREE(HYD_server_info.port_range);

    if (HYD_server_info.interface_env_name)
        HYDU_FREE(HYD_server_info.interface_env_name);

    if (HYD_server_info.nameserver)
        HYDU_FREE(HYD_server_info.nameserver);

    if (HYD_server_info.local_hostname)
        HYDU_FREE(HYD_server_info.local_hostname);

    if (HYD_server_info.node_list)
        HYDU_free_node_list(HYD_server_info.node_list);

    if (HYD_server_info.pg_list.proxy_list)
        HYDU_free_proxy_list(HYD_server_info.pg_list.proxy_list);

    if (HYD_server_info.pg_list.next)
        HYDU_free_pg_list(HYD_server_info.pg_list.next);

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
    HYDU_dump_noprefix(stdout, "  Base path: %s\n", HYD_server_info.base_path);
    HYDU_dump_noprefix(stdout, "  Launcher: %s\n", HYD_server_info.user_global.launcher);
    HYDU_dump_noprefix(stdout, "  Debug level: %d\n", HYD_server_info.user_global.debug);
    HYDU_dump_noprefix(stdout, "  Enable X: %d\n", HYD_server_info.user_global.enablex);

    HYDU_dump_noprefix(stdout, "\n");
    HYDU_dump_noprefix(stdout, "  Global environment:\n");
    HYDU_dump_noprefix(stdout, "  -------------------\n");
    for (env = HYD_server_info.user_global.global_env.inherited; env; env = env->next)
        HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);

    if (HYD_server_info.user_global.global_env.system) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  Hydra internal environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------------\n");
        for (env = HYD_server_info.user_global.global_env.system; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    if (HYD_server_info.user_global.global_env.user) {
        HYDU_dump_noprefix(stdout, "\n");
        HYDU_dump_noprefix(stdout, "  User set environment:\n");
        HYDU_dump_noprefix(stdout, "  ---------------------\n");
        for (env = HYD_server_info.user_global.global_env.user; env; env = env->next)
            HYDU_dump_noprefix(stdout, "    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_dump_noprefix(stdout, "\n\n");

    HYDU_dump_noprefix(stdout, "    Proxy information:\n");
    HYDU_dump_noprefix(stdout, "    *********************\n");
    i = 1;
    for (proxy = HYD_server_info.pg_list.proxy_list; proxy; proxy = proxy->next) {
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

HYD_status HYD_uiu_stdout_cb(int pgid, int proxy_id, int rank, void *_buf, int buflen)
{
    int sent, closed, mark, i;
    char *buf = (char *) _buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_ui_info.prepend_rank == 0) {
        status = HYDU_sock_write(STDOUT_FILENO, buf, buflen, &sent, &closed);
        HYDU_ERR_POP(status, "unable to write data to stdout\n");
        HYDU_ASSERT(!closed, status);
    }
    else {
        mark = 0;
        for (i = 0; i < buflen; i++) {
            if (buf[i] == '\n' || i == buflen - 1) {
                HYDU_dump_noprefix(stdout, "[%d] ", rank);
                status = HYDU_sock_write(STDOUT_FILENO, (const void *) &buf[mark],
                                         i - mark + 1, &sent, &closed);
                HYDU_ERR_POP(status, "unable to write data to stdout\n");
                HYDU_ASSERT(!closed, status);
                mark = i + 1;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uiu_stderr_cb(int pgid, int proxy_id, int rank, void *_buf, int buflen)
{
    int sent, closed, mark, i;
    char *buf = (char *) _buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_ui_info.prepend_rank == 0) {
        status = HYDU_sock_write(STDERR_FILENO, buf, buflen, &sent, &closed);
        HYDU_ERR_POP(status, "unable to write data to stderr\n");
        HYDU_ASSERT(!closed, status);
    }
    else {
        mark = 0;
        for (i = 0; i < buflen; i++) {
            if (buf[i] == '\n' || i == buflen - 1) {
                HYDU_dump_noprefix(stderr, "[%d] ", rank);
                status = HYDU_sock_write(STDERR_FILENO, (const void *) &buf[mark],
                                         i - mark + 1, &sent, &closed);
                HYDU_ERR_POP(status, "unable to write data to stderr\n");
                HYDU_ASSERT(!closed, status);
                mark = i + 1;
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
