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

HYD_Handle handle;
static char *pmi_port_str = NULL;

static void launch_helper(void *args)
{
    struct HYD_Partition *partition = (struct HYD_Partition *) args;
    int first_partition = (partition == handle.partition_list);
    enum HYD_PMCD_pmi_proxy_cmds cmd;
    int i, list_len, arg_len;
    HYD_Status status = HYD_SUCCESS;

    /*
     * Here are the steps we will follow:
     *
     * 1. Put all the arguments to pass in to a string list.
     *
     * 2. Connect to the proxy (this will be our primary control
     *    socket).
     *
     * 3. Read this string list and write the following to the socket:
     *    (a) The PROC_INFO command.
     *    (b) Integer sized data with the number of arguments to
     *        follow.
     *    (c) For each argument to pass, first send an integer which
     *        tells the proxy how many bytes are coming in that
     *        argument.
     *
     * 4. Open two new sockets and connect them to the proxy.
     *
     * 5. On the first new socket, send USE_AS_STDOUT and the second
     *    send USE_AS_STDERR.
     *
     * 6. For PMI_ID "0", open a separate socket and send the
     *    USE_AS_STDIN command on it.
     *
     * 7. We need to figure out what to do with the LAUNCH_JOB
     *    command; since it's going on a different socket, it might go
     *    out-of-order. Maybe a state machine on the proxy to see if
     *    it got all the information it needs to launch the job would
     *    work.
     */

    status = HYDU_sock_connect(partition->sa, &partition->control_fd);
    HYDU_ERR_POP(status, "unable to connect to proxy\n");

    cmd = PROC_INFO;
    status = HYDU_sock_write(partition->control_fd, &cmd,
                             sizeof(enum HYD_PMCD_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* Check how many arguments we have */
    list_len = HYDU_strlist_lastidx(partition->proxy_args);
    status = HYDU_sock_write(partition->control_fd, &list_len, sizeof(int));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* Convert the string list to parseable data and send */
    for (i = 0; partition->proxy_args[i]; i++) {
        arg_len = strlen(partition->proxy_args[i]) + 1;

        status = HYDU_sock_write(partition->control_fd, &arg_len, sizeof(int));
        HYDU_ERR_POP(status, "unable to write data to proxy\n");

        status = HYDU_sock_write(partition->control_fd, partition->proxy_args[i], arg_len);
        HYDU_ERR_POP(status, "unable to write data to proxy\n");
    }

    /* Create an stdout socket */
    status = HYDU_sock_connect(partition->sa, &partition->out);
    HYDU_ERR_POP(status, "unable to connect to proxy\n");

    cmd = USE_AS_STDOUT;
    status = HYDU_sock_write(partition->out, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* Create an stderr socket */
    status = HYDU_sock_connect(partition->sa, &partition->err);
    HYDU_ERR_POP(status, "unable to connect to proxy\n");

    cmd = USE_AS_STDERR;
    status = HYDU_sock_write(partition->err, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* If rank 0 is here, create an stdin socket */
    if (first_partition) {
        status = HYDU_sock_connect(partition->sa, &handle.in);
        HYDU_ERR_POP(status, "unable to connect to proxy\n");

        cmd = USE_AS_STDIN;
        status = HYDU_sock_write(handle.in, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
        HYDU_ERR_POP(status, "unable to write data to proxy\n");
    }

  fn_exit:
    return;

  fn_fail:
    goto fn_exit;
}

static HYD_Status fill_in_proxy_args(void)
{
    int i, arg, process_id;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_Env_t *env;
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    struct HYD_Partition_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    handle.one_pass_count = 0;
    for (partition = handle.partition_list; partition; partition = partition->next)
        handle.one_pass_count += partition->total_proc_count;

    /* Create the arguments list for each proxy */
    process_id = 0;
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next) {

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

        partition->proxy_args[arg++] = HYDU_strdup("--pmi-port-str");
        partition->proxy_args[arg++] = HYDU_strdup(pmi_port_str);

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

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

/* Local proxy is a proxy that is local to this process */
static HYD_Status launch_procs_in_runtime_mode(void)
{
    HYD_Status status = HYD_SUCCESS;

    status = fill_in_proxy_args();
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

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

static HYD_Status boot_proxies(int launch_in_foreground)
{
    HYD_Status status = HYD_SUCCESS;
    int i, arg;
    char *path_str[HYD_NUM_TMP_STRINGS];
    struct HYD_Partition *partition;

    handle.one_pass_count = 0;
    for (partition = handle.partition_list; partition; partition = partition->next)
        handle.one_pass_count += partition->total_proc_count;

    /* Create the arguments list for each proxy */
    for (partition = handle.partition_list; partition; partition = partition->next) {

        arg = HYDU_strlist_lastidx(partition->proxy_args);
        i = 0;
        path_str[i++] = HYDU_strdup(handle.base_path);
        path_str[i++] = HYDU_strdup("pmi_proxy");
        path_str[i] = NULL;
        status = HYDU_str_alloc_and_join(path_str, &partition->proxy_args[arg++]);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(path_str);

        partition->proxy_args[arg++] = HYDU_strdup("--persistent-mode");
        partition->proxy_args[arg++] = HYDU_strdup("--proxy-port");
        partition->proxy_args[arg++] = HYDU_int_to_str(handle.proxy_port);
        if (launch_in_foreground) {
            partition->proxy_args[arg++] = HYDU_strdup("--proxy-foreground");
        }
        partition->proxy_args[arg++] = NULL;

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

static HYD_Status shutdown_proxies(void)
{
    struct HYD_Partition *partition;
    enum HYD_PMCD_pmi_proxy_cmds cmd;
    int fd;
    HYD_Status status = HYD_SUCCESS;

    for (partition = handle.partition_list; partition; partition = partition->next) {
        status = HYDU_sock_connect(partition->sa, &fd);
        if (status != HYD_SUCCESS) {
            /* Don't abort. Try to shutdown as many proxies as possible */
            HYDU_Error_printf("Unable to connect to proxy at %s\n", partition->name);
            continue;
        }

        cmd = PROXY_SHUTDOWN;
        status = HYDU_sock_write(fd, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
        HYDU_ERR_POP(status, "unable to write data to proxy\n");

        close(fd);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status launch_procs_in_persistent_mode(void)
{
    struct HYD_Partition *partition;
    int len, id;
    struct HYD_Thread_context *thread_context = NULL;
    HYD_Status status = HYD_SUCCESS;

    status = fill_in_proxy_args();
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

    /* Though we don't use the bootstrap server right now, we still
     * initialize it, as we need to query it for information
     * sometimes. */
    status = HYD_BSCI_init(handle.bootstrap);
    HYDU_ERR_POP(status, "bootstrap server initialization failed\n");

    len = 0;
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next)
        len++;

    HYDU_CALLOC(thread_context, struct HYD_Thread_context *, len,
                sizeof(struct HYD_Thread_context), status);
    if (!thread_context)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "Unable to allocate memory for thread context\n");

    id = 0;
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next) {
        HYDU_create_thread(launch_helper, (void *) partition, &thread_context[id]);
        id++;
    }

    id = 0;
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next) {
        HYDU_join_thread(thread_context[id]);

        status = HYD_DMX_register_fd(1, &partition->control_fd, HYD_STDOUT, partition,
                                     HYD_PMCD_pmi_serv_control_cb);
        HYDU_ERR_POP(status, "unable to register control fd\n");

        id++;
    }

  fn_exit:
    if (thread_context)
        HYDU_FREE(thread_context);
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
    int listenfd;
    char *port_range, *sport;
    uint16_t port;
    char hostname[MAX_HOSTNAME_LEN];
    HYD_Env_t *env;
    struct HYD_Partition *partition;
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
    status = HYDU_sock_listen(&listenfd, port_range, &port);
    HYDU_ERR_POP(status, "unable to listen on port\n");

    /* Register the listening socket with the demux engine */
    status = HYD_DMX_register_fd(1, &listenfd, HYD_STDOUT, NULL, HYD_PMCD_pmi_connect_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Create a port string for MPI processes to use to connect to */
    if (gethostname(hostname, MAX_HOSTNAME_LEN) < 0)
        HYDU_ERR_SETANDJUMP2(status, HYD_SOCK_ERROR,
                             "gethostname error (hostname: %s; errno: %d)\n", hostname, errno);

    sport = HYDU_int_to_str(port);
    HYDU_MALLOC(pmi_port_str, char *, strlen(hostname) + 1 + strlen(sport) + 1, status);
    HYDU_snprintf(pmi_port_str, strlen(hostname) + 1 + strlen(sport) + 1,
                  "%s:%s", hostname, sport);
    HYDU_FREE(sport);
    HYDU_Debug("Process manager listening on PMI port %s\n", pmi_port_str);

    /* Initialize PMI */
    status = HYD_PMCD_pmi_init();
    HYDU_ERR_POP(status, "unable to create process group\n");

    /* For each partition, get the appropriate sockaddr to connect to */
    for (partition = handle.partition_list; partition; partition = partition->next) {
        status = HYDU_sock_gethostbyname(partition->name, &partition->sa, handle.proxy_port);
        HYDU_ERR_POP(status, "unable to get sockaddr information\n");
    }

    if (handle.launch_mode == HYD_LAUNCH_RUNTIME) {
        status = launch_procs_in_runtime_mode();
        HYDU_ERR_POP(status, "error launching procs in runtime mode\n");
    }
    else if (handle.launch_mode == HYD_LAUNCH_BOOT ||
             handle.launch_mode == HYD_LAUNCH_BOOT_FOREGROUND) {
        status = boot_proxies((handle.launch_mode == HYD_LAUNCH_BOOT) ? 0 : 1);
        HYDU_ERR_POP(status, "error booting proxies\n");
    }
    else if (handle.launch_mode == HYD_LAUNCH_SHUTDOWN) {
        status = shutdown_proxies();
        HYDU_ERR_POP(status, "error shutting down proxies\n");
    }
    else if (handle.launch_mode == HYD_LAUNCH_PERSISTENT) {
        status = launch_procs_in_persistent_mode();
        HYDU_ERR_POP(status, "error launching procs in persistent mode\n");
    }

  fn_exit:
    if (pmi_port_str)
        HYDU_FREE(pmi_port_str);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCI_wait_for_completion(void)
{
    struct HYD_Partition *partition;
    int sockets_open, all_procs_exited;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((handle.launch_mode == HYD_LAUNCH_BOOT) || (handle.launch_mode == HYD_LAUNCH_SHUTDOWN)) {
        status = HYD_SUCCESS;
    }
    else {
        while (1) {
            /* Wait for some event to occur */
            status = HYD_DMX_wait_for_event(HYDU_time_left(handle.start, handle.timeout));
            HYDU_ERR_POP(status, "error waiting for event\n");

            /* Check to see if there's any open read socket left; if
             * there are, we will just wait for more events. */
            sockets_open = 0;
            for (partition = handle.partition_list; partition && partition->exec_list;
                 partition = partition->next) {
                if (partition->out != -1 || partition->err != -1) {
                    sockets_open++;
                    break;
                }
            }

            if (sockets_open && HYDU_time_left(handle.start, handle.timeout))
                continue;

            break;
        }

        /* The bootstrap will wait for all processes to terminate */
        if (handle.launch_mode == HYD_LAUNCH_RUNTIME) {
            status = HYD_BSCI_wait_for_completion();
            if (status != HYD_SUCCESS) {
                status = HYD_PMCD_pmi_serv_cleanup();
                HYDU_ERR_POP(status, "process manager cannot cleanup processes\n");
            }
        }
        else if (handle.launch_mode == HYD_LAUNCH_PERSISTENT) {
            do {
                /* Check if the exit status has already arrived */
                all_procs_exited = 1;
                for (partition = handle.partition_list; partition && partition->exec_list;
                     partition = partition->next) {
                    if (partition->exit_status == -1) {
                        all_procs_exited = 0;
                        break;
                    }
                }

                if (all_procs_exited)
                    break;

                /* If not, wait for some event to occur */
                status = HYD_DMX_wait_for_event(HYDU_time_left(handle.start, handle.timeout));
                HYDU_ERR_POP(status, "error waiting for event\n");
            } while (1);
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
