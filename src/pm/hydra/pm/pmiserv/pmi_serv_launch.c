/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmci.h"
#include "pmi_handle.h"
#include "bsci.h"
#include "pmi_serv.h"

static char *pmi_port = NULL;
static int pmi_id = -1;

static HYD_status fill_in_exec_launch_info(void)
{
    int i, arg, process_id;
    int inherited_env_count, user_env_count, system_env_count;
    int exec_count, total_args;
    static int proxy_count = 0;
    HYD_env_t *env;
    struct HYD_proxy *proxy;
    struct HYD_proxy_exec *exec;
    HYD_status status = HYD_SUCCESS;

    /* Create the arguments list for each proxy */
    process_id = 0;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
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

        proxy->exec_launch_info[arg++] = HYDU_strdup("--wdir");
        proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.wdir);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-port");
        proxy->exec_launch_info[arg++] = HYDU_strdup(pmi_port);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--pmi-id");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(pmi_id);

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
        proxy->exec_launch_info[arg++] = HYDU_strdup("--genv-prop");
        proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.global_env.prop);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--start-pid");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(proxy->start_pid);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--proxy-core-count");
        proxy->exec_launch_info[arg++] = HYDU_int_to_str(proxy->node.core_count);
        proxy->exec_launch_info[arg++] = NULL;

        /* Now pass the local executable information */
        for (exec = proxy->exec_list; exec; exec = exec->next) {
            arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec");

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
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_launch_procs(void)
{
    struct HYD_proxy *proxy;
    struct HYD_node *node_list, *node, *tnode;
    char *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL }, *tmp = NULL, *control_port = NULL;
    int zero = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_set_common_signals(HYD_pmcd_pmi_serv_signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    /* Initialize PMI */
    tmp = getenv("PMI_PORT");
    if (!tmp) { /* PMI_PORT not already set; create one */
        /* pass PGID 0 as a user parameter to the PMI connect handler */
        status = HYDU_sock_create_and_listen_portstr(HYD_handle.port_range, &pmi_port,
                                                     HYD_pmcd_pmi_connect_cb, &zero);
        HYDU_ERR_POP(status, "unable to create PMI port\n");
        pmi_id = -1;
    }
    else {
        if (HYD_handle.user_global.debug)
            HYDU_dump(stdout, "someone else already set PMI port\n");
        pmi_port = HYDU_strdup(tmp);
        tmp = getenv("PMI_ID");
        if (!tmp)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI_PORT set but not PMI_ID\n");
        pmi_id = atoi(tmp);
    }
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "PMI port: %s; PMI ID: %d\n", pmi_port, pmi_id);

    status = HYD_pmcd_pmi_init();
    HYDU_ERR_POP(status, "unable to create process group\n");

    /* Copy the host list to pass to the bootstrap server */
    node_list = NULL;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        HYDU_alloc_node(&node);
        node->hostname = HYDU_strdup(proxy->node.hostname);
        node->core_count = proxy->node.core_count;
        node->next = NULL;

        if (node_list == NULL) {
            node_list = node;
        }
        else {
            for (tnode = node_list; tnode->next; tnode = tnode->next);
            tnode->next = node;
        }
    }

    status = HYDU_sock_create_and_listen_portstr(HYD_handle.port_range, &control_port,
                                                 HYD_pmcd_pmi_serv_control_connect_cb, NULL);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    status = HYD_pmcd_pmi_fill_in_proxy_args(proxy_args, control_port, 0);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

    status = fill_in_exec_launch_info();
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_bsci_launch_procs(proxy_args, node_list, HYD_handle.stdout_cb,
                                    HYD_handle.stderr_cb);
    HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");

  fn_exit:
    if (pmi_port)
        HYDU_FREE(pmi_port);
    if (control_port)
        HYDU_FREE(control_port);
    HYDU_free_strlist(proxy_args);
    HYDU_free_node_list(node_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmci_wait_for_completion(int timeout)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_wait_for_completion(timeout);
    if (status == HYD_TIMED_OUT) {
        status = HYD_pmcd_pmi_serv_cleanup();
        HYDU_ERR_POP(status, "cleanup of processes failed\n");
    }
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
