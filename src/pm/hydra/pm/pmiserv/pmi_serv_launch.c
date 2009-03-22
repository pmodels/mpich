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
HYD_Handle handle;

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
    int i, arg, process_id;
    char hostname[MAX_HOSTNAME_LEN];
    HYD_Env_t *env;
    char *path_str[HYDU_NUM_JOIN_STR];
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    struct HYD_Partition_segment *segment;
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
    status = HYD_DMX_register_fd(1, &HYD_PMCD_pmi_serv_listenfd, HYD_STDOUT,
                                 HYD_PMCD_pmi_serv_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Create a port string for MPI processes to use to connect to */
    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR,
                             "gethostname error (hostname: %s; errno: %d)\n", hostname, errno);

    sport = HYDU_int_to_str(port);

    HYDU_MALLOC(port_str, char *, strlen(hostname) + 1 + strlen(sport) + 1, status);
    MPIU_Snprintf(port_str, strlen(hostname) + 1 + strlen(sport) + 1,
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

    handle.one_pass_count = 0;
    for (partition = handle.partition_list; partition; partition = partition->next)
        handle.one_pass_count += partition->total_proc_count;

    /* Create the arguments list for each proxy */
    process_id = 0;
    for (partition = handle.partition_list; partition; partition = partition->next) {

        arg = HYDU_strlist_lastidx(partition->proxy_args);
        i = 0;
        path_str[i++] = MPIU_Strdup(handle.base_path);
        path_str[i++] = MPIU_Strdup("pmi_proxy");
        path_str[i] = NULL;
        status = HYDU_str_alloc_and_join(path_str, &partition->proxy_args[arg++]);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(path_str);

        partition->proxy_args[arg++] = MPIU_Strdup("--proxy-port");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.proxy_port);

        partition->proxy_args[arg++] = MPIU_Strdup("--one-pass-count");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.one_pass_count);

        partition->proxy_args[arg++] = MPIU_Strdup("--wdir");
        partition->proxy_args[arg++] = MPIU_Strdup(handle.wdir);

        partition->proxy_args[arg++] = MPIU_Strdup("--binding");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.binding);

        /* Pass the global environment separately, instead of for each
         * executable, as an optimization */
        partition->proxy_args[arg++] = MPIU_Strdup("--global-env");
        for (i = 0, env = handle.system_env; env; env = env->next, i++);
        for (env = handle.prop_env; env; env = env->next, i++);
        partition->proxy_args[arg++] = HYDU_int_to_str(i);
        partition->proxy_args[arg++] = NULL;
        HYDU_list_append_env_to_str(handle.system_env, partition->proxy_args);
        HYDU_list_append_env_to_str(handle.prop_env, partition->proxy_args);

        /* Pass the segment information */
        for (segment = partition->segment_list; segment; segment = segment->next) {
            arg = HYDU_strlist_lastidx(partition->proxy_args);
            partition->proxy_args[arg++] = MPIU_Strdup("--segment");

            partition->proxy_args[arg++] = MPIU_Strdup("--segment-start-pid");
            partition->proxy_args[arg++] = HYDU_int_to_str(segment->start_pid);

            partition->proxy_args[arg++] = MPIU_Strdup("--segment-proc-count");
            partition->proxy_args[arg++] = HYDU_int_to_str(segment->proc_count);
            partition->proxy_args[arg++] = NULL;
        }

        /* Now pass the local executable information */
        for (exec = partition->exec_list; exec; exec = exec->next) {
            arg = HYDU_strlist_lastidx(partition->proxy_args);
            partition->proxy_args[arg++] = MPIU_Strdup("--exec");

            partition->proxy_args[arg++] = MPIU_Strdup("--exec-proc-count");
            partition->proxy_args[arg++] = HYDU_int_to_str(exec->proc_count);
            partition->proxy_args[arg++] = NULL;

            arg = HYDU_strlist_lastidx(partition->proxy_args);
            partition->proxy_args[arg++] = MPIU_Strdup("--exec-local-env");
            for (i = 0, env = exec->prop_env; env; env = env->next, i++);
            HYDU_ERR_POP(status, "unable to convert int to string\n");
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
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCI_wait_for_completion(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCI_wait_for_completion();
    if (status != HYD_SUCCESS) {
        status = HYD_PMCD_pmi_serv_cleanup();
        HYDU_ERR_POP(status, "process manager cannot cleanup processes\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
