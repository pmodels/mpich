/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmci.h"
#include "pmcu_pmi.h"
#include "bsci.h"
#include "demux.h"
#include "central.h"

int HYD_PMCD_Central_listenfd;
HYD_Handle handle;

/*
 * HYD_PMCI_Launch_procs: Here are the steps we follow:
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
HYD_Status HYD_PMCI_Launch_procs(void)
{
    char *port_range, *port_str, *sport, *str;
    uint16_t port;
    int i, arg;
    int process_id, group_id;
    char hostname[MAX_HOSTNAME_LEN];
    HYD_Env_t *env;
    char *path_str[HYDU_NUM_JOIN_STR];
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_Set_common_signals(HYD_PMCD_Central_signal_cb);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("signal utils returned error when trying to set signal\n");
        goto fn_fail;
    }

    /* Check if the user wants us to use a port within a certain
     * range. */
    port_range = getenv("MPIEXEC_PORTRANGE");
    if (!port_range)
        port_range = getenv("MPIEXEC_PORT_RANGE");
    if (!port_range)
        port_range = getenv("MPICH_PORT_RANGE");

    /* Listen on a port in the port range */
    port = 0;
    status = HYDU_Sock_listen(&HYD_PMCD_Central_listenfd, port_range, &port);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("sock utils returned listen error\n");
        goto fn_fail;
    }

    /* Register the listening socket with the demux engine */
    status = HYD_DMX_Register_fd(1, &HYD_PMCD_Central_listenfd, HYD_STDOUT,
                                 HYD_PMCD_Central_cb);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("demux engine returned error for registering fd\n");
        goto fn_fail;
    }

    /* Create a port string for MPI processes to use to connect to */
    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0) {
        HYDU_Error_printf("gethostname error (hostname: %s; errno: %d)\n", hostname, errno);
        status = HYD_SOCK_ERROR;
        goto fn_fail;
    }

    status = HYDU_String_int_to_str(port, &sport);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("String utils returned error while converting int to string\n");
        goto fn_fail;
    }
    HYDU_MALLOC(port_str, char *, strlen(hostname) + 1 + strlen(sport) + 1, status);
    MPIU_Snprintf(port_str, strlen(hostname) + 1 + strlen(sport) + 1,
                  "%s:%s", hostname, sport);
    HYDU_FREE(sport);
    HYDU_Debug("Process manager listening on PMI port %s\n", port_str);

    status = HYDU_Env_create(&env, "PMI_PORT", port_str);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to create env\n");
        goto fn_fail;
    }
    status = HYDU_Env_add_to_list(&handle.system_env, *env);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to add env to list\n");
        goto fn_fail;
    }
    HYDU_Env_free(env);
    HYDU_FREE(port_str);

    /* Create a process group for the MPI processes in this
     * comm_world */
    status = HYD_PMCU_Create_pg();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("pm utils returned error creating process group\n");
        goto fn_fail;
    }

    /* Create the arguments list for each proxy */
    process_id = 0;
    group_id = 0;
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        for (partition = proc_params->partition; partition; partition = partition->next) {

            partition->group_id = group_id++;
            partition->group_rank = 0;

            for (arg = 0; partition->args[arg]; arg++);
            i = 0;
            if (handle.base_path[0] && (handle.base_path[0] != '/')) {
                /* This should be a relative path; add the wdir as well */
                path_str[i++] = MPIU_Strdup(handle.wdir);
                path_str[i++] = MPIU_Strdup("/");
            }
            path_str[i++] = MPIU_Strdup(handle.base_path);
            path_str[i++] = MPIU_Strdup("hydproxy");
            path_str[i] = NULL;
            status = HYDU_String_alloc_and_join(path_str, &partition->args[arg]);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("String utils returned error when joining strings\n");
                goto fn_fail;
            }
            arg++;

            HYDU_Free_args(path_str);

            status = HYDU_String_int_to_str(partition->proc_count, &str);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf
                    ("String utils returned error while converting int to string\n");
                goto fn_fail;
            }
            partition->args[arg++] = MPIU_Strdup("--proc-count");
            partition->args[arg++] = MPIU_Strdup(str);

            partition->args[arg++] = MPIU_Strdup("--partition");
            partition->args[arg++] = MPIU_Strdup(partition->name);
            partition->args[arg++] = MPIU_Strdup(str);
            HYDU_FREE(str);

            status = HYDU_String_int_to_str(process_id, &str);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf
                    ("String utils returned error while converting int to string\n");
                goto fn_fail;
            }
            partition->args[arg++] = MPIU_Strdup("--pmi-id");
            partition->args[arg++] = MPIU_Strdup(str);
            HYDU_FREE(str);

            status = HYDU_String_int_to_str(handle.proxy_port, &str);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf
                    ("String utils returned error while converting int to string\n");
                goto fn_fail;
            }
            partition->args[arg++] = MPIU_Strdup("--proxy-port");
            partition->args[arg++] = MPIU_Strdup(str);
            HYDU_FREE(str);

            partition->args[arg++] = MPIU_Strdup("--wdir");
            partition->args[arg++] = MPIU_Strdup(handle.wdir);

            partition->args[arg++] = MPIU_Strdup("--environment");
            i = 0;
            for (env = handle.system_env; env; env = env->next)
                i++;
            for (env = handle.prop_env; env; env = env->next)
                i++;
            for (env = proc_params->prop_env; env; env = env->next)
                i++;
            status = HYDU_String_int_to_str(i, &str);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf
                    ("String utils returned error while converting int to string\n");
                goto fn_fail;
            }
            partition->args[arg++] = MPIU_Strdup(str);
            partition->args[arg++] = NULL;

            HYDU_Append_env(handle.system_env, partition->args);
            HYDU_Append_env(handle.prop_env, partition->args);
            HYDU_Append_env(proc_params->prop_env, partition->args);

            for (arg = 0; partition->args[arg]; arg++);
            partition->args[arg] = NULL;
            HYDU_Append_exec(proc_params->exec, partition->args);

            if (handle.debug) {
                HYDU_Debug("Executable passed to the bootstrap: ");
                HYDU_Dump_args(partition->args);
            }

            process_id += partition->proc_count;
        }
    }

    /* Initialize the bootstrap server and ask it to launch the
     * processes. */
    status = HYD_BSCI_init(handle.bootstrap);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("bootstrap server initialization failed\n");
        goto fn_fail;
    }

    status = HYD_BSCI_launch_procs();
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("bootstrap server returned error launching processes\n");
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCI_Wait_for_completion(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCI_wait_for_completion();
    if (status != HYD_SUCCESS) {
        status = HYD_PMCD_Central_cleanup();
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("process manager device returned error for process cleanup\n");
            goto fn_fail;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
