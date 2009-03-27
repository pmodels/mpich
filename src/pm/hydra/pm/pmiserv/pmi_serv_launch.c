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

int HYD_PMCD_pmi_serv_listenfd;
extern HYD_Handle handle;

/* Local proxy is a proxy that is local to this process */
static HYD_Status launch_procs_with_local_proxy(void)
{
    HYD_Status status = HYD_SUCCESS;
    int i, arg, process_id;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_Env_t *env;
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    struct HYD_Partition_segment *segment;

    handle.one_pass_count = 0;
    for (partition = handle.partition_list; partition; partition = partition->next)
        handle.one_pass_count += partition->total_proc_count;

    /* Create the arguments list for each proxy */
    process_id = 0;
    for (partition = handle.partition_list; partition; partition = partition->next) {

        if (partition->exec_list == NULL)
            break;

        arg = HYDU_strlist_lastidx(partition->proxy_args);
        i = 0;
        path_str[i++] = HYDU_strdup(handle.base_path);
        path_str[i++] = HYDU_strdup("pmi_proxy");
        path_str[i] = NULL;
        status = HYDU_str_alloc_and_join(path_str, &partition->proxy_args[arg++]);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(path_str);

        partition->proxy_args[arg++] = HYDU_strdup("--proxy-port");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.proxy_port);

        partition->proxy_args[arg++] = HYDU_strdup("--one-pass-count");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.one_pass_count);

        partition->proxy_args[arg++] = HYDU_strdup("--wdir");
        partition->proxy_args[arg++] = HYDU_strdup(handle.wdir);

        partition->proxy_args[arg++] = HYDU_strdup("--binding");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.binding);
        if (handle.user_bind_map)
            partition->proxy_args[arg++] = HYDU_strdup(handle.user_bind_map);
        else if (partition->user_bind_map)
            partition->proxy_args[arg++] = HYDU_strdup(partition->user_bind_map);
        else
            partition->proxy_args[arg++] = HYDU_strdup("HYDRA_NO_USER_MAP");

        /* Pass the global environment separately, instead of for each
         * executable, as an optimization */
        partition->proxy_args[arg++] = HYDU_strdup("--global-env");
        for (i = 0, env = handle.system_env; env; env = env->next, i++);
        for (env = handle.prop_env; env; env = env->next, i++);
        partition->proxy_args[arg++] = HYDU_int_to_str(i);
        partition->proxy_args[arg++] = NULL;
        HYDU_list_append_env_to_str(handle.system_env, partition->proxy_args);
        HYDU_list_append_env_to_str(handle.prop_env, partition->proxy_args);

        /* Pass the segment information */
        for (segment = partition->segment_list; segment; segment = segment->next) {
            arg = HYDU_strlist_lastidx(partition->proxy_args);
            partition->proxy_args[arg++] = HYDU_strdup("--segment");

            partition->proxy_args[arg++] = HYDU_strdup("--segment-start-pid");
            partition->proxy_args[arg++] = HYDU_int_to_str(segment->start_pid);

            partition->proxy_args[arg++] = HYDU_strdup("--segment-proc-count");
            partition->proxy_args[arg++] = HYDU_int_to_str(segment->proc_count);
            partition->proxy_args[arg++] = NULL;
        }

        /* Now pass the local executable information */
        for (exec = partition->exec_list; exec; exec = exec->next) {
            arg = HYDU_strlist_lastidx(partition->proxy_args);
            partition->proxy_args[arg++] = HYDU_strdup("--exec");

            partition->proxy_args[arg++] = HYDU_strdup("--exec-proc-count");
            partition->proxy_args[arg++] = HYDU_int_to_str(exec->proc_count);
            partition->proxy_args[arg++] = NULL;

            arg = HYDU_strlist_lastidx(partition->proxy_args);
            partition->proxy_args[arg++] = HYDU_strdup("--exec-local-env");
            for (i = 0, env = exec->prop_env; env; env = env->next, i++);
            partition->proxy_args[arg++] = HYDU_int_to_str(i);
            partition->proxy_args[arg++] = NULL;
            HYDU_list_append_env_to_str(exec->prop_env, partition->proxy_args);

            HYDU_list_append_strlist(exec->exec, partition->proxy_args);

            process_id += exec->proc_count;
        }

        if (handle.debug) {
            HYDU_Debug("Executable passed to the bootstrap: ");
            HYDU_print_strlist(partition->proxy_args);
        }
    }

    /* Initialize the bootstrap server and ask it to launch the
     * processes. */
    status = HYD_BSCI_init(handle.bootstrap);
    HYDU_ERR_POP(status, "bootstrap server initialization failed\n");

    status = HYD_BSCI_launch_procs();
    HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/* Request remote proxies to shutdown */
static HYD_Status shutdown_remote_proxies(void)
{
    char shutdown_proxies_cmd[HYD_PMCD_MAX_CMD_LEN];
    struct HYD_Partition *partition = NULL;
    int HYD_PMCD_pmi_proxy_connfd = -1;
    HYD_Status status = HYD_SUCCESS;

    for (partition = handle.partition_list; partition; partition = partition->next) {
        status = HYDU_sock_connect(partition->name, handle.pproxy_port,
                                   &HYD_PMCD_pmi_proxy_connfd);
        HYDU_ERR_POP(status, "Error connecting to remote proxy");

        /* Create shutdown command */
        HYDU_snprintf(shutdown_proxies_cmd, HYD_PMCD_MAX_CMD_LEN,
                      "%s=%s %c\n", "cmd", HYD_PMCD_CMD_SHUTDOWN, HYD_PMCD_CMD_SEP_CHAR);

        status = HYDU_sock_writeline(HYD_PMCD_pmi_proxy_connfd, shutdown_proxies_cmd,
                                     strlen(shutdown_proxies_cmd));
        HYDU_ERR_POP(status, "Error writing the launch procs command\n");

        /* FIXME: Read result */
        partition->out = HYD_PMCD_pmi_proxy_connfd;
        partition->err = -1;
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/* Remote proxy is a proxy external to this process */
static HYD_Status launch_procs_with_remote_proxy(void)
{
    HYD_Status status = HYD_SUCCESS;
    char launch_procs_cmd[HYD_PMCD_MAX_CMD_LEN];
    char env_list[HYD_PMCD_MAX_CMD_LEN];        /* FIXME: Wrong *MAX*... */
    int env_list_len = 0;
    char *p = NULL;
    struct HYD_Partition *partition = NULL;
    struct HYD_Partition_exec *exec = NULL;
    struct HYD_Env *env = NULL;
    int HYD_PMCD_pmi_proxy_connfd = -1;

    for (partition = handle.partition_list; partition; partition = partition->next) {
        status = HYDU_sock_connect(partition->name, handle.pproxy_port,
                                   &HYD_PMCD_pmi_proxy_connfd);
        HYDU_ERR_POP(status, "Error connecting to remote proxy");

        exec = partition->exec_list;

        /* FIXME: Create a util func for converting env list to a string */
        env = handle.system_env;
        p = env_list;
        *p = '\0';
        env_list_len = HYD_PMCD_MAX_CMD_LEN;
        while (env) {
            HYDU_snprintf(p, env_list_len, "%s=%s %c",
                          env->env_name, env->env_value, HYD_PMCD_CMD_ENV_SEP_CHAR);
            env_list_len -= strlen(p);
            p += strlen(p);
            env = env->next;
        }
        /* Create launch command */
        HYDU_snprintf(launch_procs_cmd, HYD_PMCD_MAX_CMD_LEN,
                      "%s=%s %c %s=%s %c %s=%d %c %s=%s %c\n",
                      "cmd", HYD_PMCD_CMD_LAUNCH_PROCS, HYD_PMCD_CMD_SEP_CHAR,
                      "exec_name", exec->exec[0], HYD_PMCD_CMD_SEP_CHAR,
                      "exec_cnt", exec->proc_count, HYD_PMCD_CMD_SEP_CHAR,
                      "env", env_list, HYD_PMCD_CMD_SEP_CHAR);

        status = HYDU_sock_writeline(HYD_PMCD_pmi_proxy_connfd, launch_procs_cmd,
                                     strlen(launch_procs_cmd));
        HYDU_ERR_POP(status, "Error writing the launch procs command\n");

        /* FIXME: Read result */
        partition->out = HYD_PMCD_pmi_proxy_connfd;
        partition->err = -1;
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/*
 * HYD_PMCI_launch_procs: Here are the steps we follow:
 *
 * 1. Find what all ports the user wants to allow and listen on one of
 * those ports.
 *
 * 2. Create a call-back function to accept connections and register
 * the listening socket with the demux engine.
 *
 * 3. Create a port string out of this hostname and port and add it to
 * the environment list under the variable "PMI_PORT".
 *
 * 4. Create an environment variable for PMI_ID. This is an
 * auto-incrementing variable; the bootstrap server will take care of
 * adding the process ID to the start value.
 *
 * 5. Create a process info setup and ask the bootstrap server to
 * launch the processes.
 */
HYD_Status HYD_PMCI_launch_procs(void)
{
    char *port_range, *port_str, *sport;
    uint16_t port;
    char hostname[MAX_HOSTNAME_LEN];
    HYD_Env_t *env;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_set_common_signals(HYD_PMCD_pmi_serv_signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    /* Check if the user wants us to use a port within a certain
     * range. */
    port_range = getenv("MPIEXEC_PORTRANGE");
    if (!port_range)
        port_range = getenv("MPIEXEC_PORT_RANGE");
    if (!port_range)
        port_range = getenv("MPICH_PORT_RANGE");

    /* Listen on a port in the port range */
    port = 0;
    status = HYDU_sock_listen(&HYD_PMCD_pmi_serv_listenfd, port_range, &port);
    HYDU_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYD_DMX_register_fd(1, &HYD_PMCD_pmi_serv_listenfd, HYD_STDOUT, NULL,
                                 HYD_PMCD_pmi_serv_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Create a port string for MPI processes to use to connect to */
    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR,
                             "gethostname error (hostname: %s; errno: %d)\n", hostname, errno);

    sport = HYDU_int_to_str(port);

    HYDU_MALLOC(port_str, char *, strlen(hostname) + 1 + strlen(sport) + 1, status);
    HYDU_snprintf(port_str, strlen(hostname) + 1 + strlen(sport) + 1,
                  "%s:%s", hostname, sport);
    HYDU_FREE(sport);
    HYDU_Debug("Process manager listening on PMI port %s\n", port_str);

    status = HYDU_env_create(&env, "PMI_PORT", port_str);
    HYDU_ERR_POP(status, "unable to create env\n");

    status = HYDU_append_env_to_list(*env, &handle.system_env);
    HYDU_ERR_POP(status, "unable to add env to list\n");

    HYDU_env_free(env);
    HYDU_FREE(port_str);

    /* Create a process group for the MPI processes in this
     * comm_world */
    status = HYD_PMCD_pmi_create_pg();
    HYDU_ERR_POP(status, "unable to create process group\n");

    if (handle.is_proxy_remote) {
        if (handle.is_proxy_terminator) {
            status = shutdown_remote_proxies();
            HYDU_ERR_POP(status, "Error shutting down remote proxies\n");
        }
        else {
            status = launch_procs_with_remote_proxy();
            HYDU_ERR_POP(status, "Error launching procs with remote proxy\n");
        }
    }
    else {
        status = launch_procs_with_local_proxy();
        HYDU_ERR_POP(status, "Error launching procs with local proxy\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCI_wait_for_completion(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.is_proxy_remote) {
        status = HYD_SUCCESS;
    }
    else {
        status = HYD_BSCI_wait_for_completion();
        if (status != HYD_SUCCESS) {
            status = HYD_PMCD_pmi_serv_cleanup();
            HYDU_ERR_POP(status, "process manager cannot cleanup processes\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
