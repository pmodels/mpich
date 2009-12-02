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
#include "demux.h"
#include "pmi_serv.h"

static char *pmi_port = NULL;
static int pmi_id = -1;
static char *control_port = NULL;

static HYD_status
create_and_listen_portstr(HYD_status(*callback) (int fd, HYD_event_t events, void *userp),
                          char **port_str)
{
    int listenfd;
    char *port_range, *sport;
    uint16_t port;
    char hostname[MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    /* Check if the user wants us to use a port within a certain
     * range. */
    port_range = getenv("MPIEXEC_PORTRANGE");
    if (!port_range)
        port_range = getenv("MPIEXEC_PORT_RANGE");
    if (!port_range)
        port_range = getenv("MPICH_PORT_RANGE");

    /* Listen on a port in the port range */
    port = 0;
    status = HYDU_sock_listen(&listenfd, port_range, &port);
    HYDU_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYDT_dmx_register_fd(1, &listenfd, HYD_STDOUT, NULL, callback);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Create a port string for MPI processes to use to connect to */
    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR,
                             "gethostname error (hostname: %s; errno: %d)\n", hostname, errno);

    sport = HYDU_int_to_str(port);
    HYDU_MALLOC(*port_str, char *, strlen(hostname) + 1 + strlen(sport) + 1, status);
    HYDU_snprintf(*port_str, strlen(hostname) + 1 + strlen(sport) + 1,
                  "%s:%s", hostname, sport);
    HYDU_FREE(sport);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fill_in_proxy_args(char **proxy_args)
{
    int i, arg;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    arg = 0;
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

    if (HYD_handle.user_global.enablex != -1) {
        proxy_args[arg++] = HYDU_strdup("--enable-x");
        proxy_args[arg++] = HYDU_int_to_str(HYD_handle.user_global.enablex);
    }

    proxy_args[arg++] = HYDU_strdup("--bootstrap");
    proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.bootstrap);

    if (HYD_handle.user_global.bootstrap_exec) {
        proxy_args[arg++] = HYDU_strdup("--bootstrap-exec");
        proxy_args[arg++] = HYDU_strdup(HYD_handle.user_global.bootstrap_exec);
    }

    proxy_args[arg++] = HYDU_strdup("--proxy-id");
    proxy_args[arg++] = NULL;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

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

        proxy->exec_launch_info[arg++] = HYDU_strdup("--binding");
        if (HYD_handle.user_global.binding)
            proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.binding);
        else
            proxy->exec_launch_info[arg++] = HYDU_strdup("HYDRA_NULL");

        proxy->exec_launch_info[arg++] = HYDU_strdup("--bindlib");
        proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.bindlib);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpointlib");
        proxy->exec_launch_info[arg++] = HYDU_strdup(HYD_handle.user_global.ckpointlib);

        proxy->exec_launch_info[arg++] = HYDU_strdup("--ckpoint-prefix");
        if (HYD_handle.user_global.ckpoint_prefix)
            proxy->exec_launch_info[arg++] =
                HYDU_strdup(HYD_handle.user_global.ckpoint_prefix);
        else
            proxy->exec_launch_info[arg++] = HYDU_strdup("HYDRA_NULL");

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

            arg = HYDU_strlist_lastidx(proxy->exec_launch_info);
            proxy->exec_launch_info[arg++] = HYDU_strdup("--exec-env-prop");
            proxy->exec_launch_info[arg++] = exec->env_prop ? HYDU_strdup(exec->env_prop) :
                HYDU_strdup("HYDRA_NULL");
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
    enum HYD_pmcd_pmi_proxy_cmds cmd;
    int fd;
    char *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL }, *tmp = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_set_common_signals(HYD_pmcd_pmi_serv_signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    /* Initialize PMI */
    tmp = getenv("PMI_PORT");
    if (!tmp) { /* PMI_PORT not already set; create one */
        status = create_and_listen_portstr(HYD_pmcd_pmi_connect_cb, &pmi_port);
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

    status = create_and_listen_portstr(HYD_pmcd_pmi_serv_control_connect_cb,
                                       &control_port);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "Got a proxy port string of %s\n", control_port);

    status = fill_in_proxy_args(proxy_args);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

    status = fill_in_exec_launch_info();
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_bsci_launch_procs(proxy_args, node_list, NULL, HYD_handle.stdin_cb,
                                    HYD_handle.stdout_cb, HYD_handle.stderr_cb);
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


HYD_status HYD_pmci_wait_for_completion(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDT_bsci_wait_for_completion(HYD_handle.timeout);
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
