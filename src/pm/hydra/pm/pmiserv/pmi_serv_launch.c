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

static char *pmi_port_str = NULL;
static char *proxy_port_str = NULL;

static void *launch_helper(void *args)
{
    struct HYD_Partition *partition = (struct HYD_Partition *) args;
    enum HYD_PMCD_pmi_proxy_cmds cmd;
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
     * 6. For the first process, open a separate socket and send the
     *    USE_AS_STDIN command on it.
     *
     * 7. We need to figure out what to do with the LAUNCH_JOB
     *    command; since it's going on a different socket, it might go
     *    out-of-order. Maybe a state machine on the proxy to see if
     *    it got all the information it needs to launch the job would
     *    work.
     */

    status = HYDU_sock_connect(partition->base->name, HYD_handle.proxy_port,
                               &partition->control_fd);
    HYDU_ERR_POP(status, "unable to connect to proxy\n");

    status = HYD_PMCD_pmi_send_exec_info(partition);
    HYDU_ERR_POP(status, "error sending executable info\n");

    /* Create an stdout socket */
    status = HYDU_sock_connect(partition->base->name, HYD_handle.proxy_port,
                               &partition->base->out);
    HYDU_ERR_POP(status, "unable to connect to proxy\n");

    cmd = USE_AS_STDOUT;
    status = HYDU_sock_write(partition->base->out, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* Create an stderr socket */
    status = HYDU_sock_connect(partition->base->name, HYD_handle.proxy_port,
                               &partition->base->err);
    HYDU_ERR_POP(status, "unable to connect to proxy\n");

    cmd = USE_AS_STDERR;
    status = HYDU_sock_write(partition->base->err, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
    HYDU_ERR_POP(status, "unable to write data to proxy\n");

    /* If rank 0 is here, create an stdin socket */
    if (partition->base->partition_id == 0) {
        status = HYDU_sock_connect(partition->base->name, HYD_handle.proxy_port,
                                   &partition->base->in);
        HYDU_ERR_POP(status, "unable to connect to proxy\n");

        cmd = USE_AS_STDIN;
        status = HYDU_sock_write(partition->base->in, &cmd,
                                 sizeof(enum HYD_PMCD_pmi_proxy_cmds));
        HYDU_ERR_POP(status, "unable to write data to proxy\n");
    }

  fn_exit:
    return NULL;

  fn_fail:
    goto fn_exit;
}

static HYD_Status
create_and_listen_portstr(HYD_Status(*callback) (int fd, HYD_Event_t events, void *userp),
                          char **port_str)
{
    int listenfd;
    char *port_range, *sport;
    uint16_t port;
    char hostname[MAX_HOSTNAME_LEN];
    HYD_Status status = HYD_SUCCESS;

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
    status = HYD_DMX_register_fd(1, &listenfd, HYD_STDOUT, NULL, callback);
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

static HYD_Status fill_in_proxy_args(HYD_Launch_mode_t mode, char **proxy_args)
{
    int i, arg;
    char *path_str[HYD_NUM_TMP_STRINGS];
    HYD_Status status = HYD_SUCCESS;

    if (mode != HYD_LAUNCH_RUNTIME && mode != HYD_LAUNCH_BOOT &&
        mode != HYD_LAUNCH_BOOT_FOREGROUND)
        goto fn_exit;

    arg = 0;
    i = 0;
    path_str[i++] = HYDU_strdup(HYD_handle.base_path);
    path_str[i++] = HYDU_strdup("pmi_proxy");
    path_str[i] = NULL;
    status = HYDU_str_alloc_and_join(path_str, &proxy_args[arg++]);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(path_str);

    proxy_args[arg++] = HYDU_strdup("--launch-mode");
    proxy_args[arg++] = HYDU_int_to_str(mode);

    proxy_args[arg++] = HYDU_strdup("--proxy-port");
    if (mode == HYD_LAUNCH_RUNTIME)
        proxy_args[arg++] = HYDU_strdup(proxy_port_str);
    else
        proxy_args[arg++] = HYDU_int_to_str(HYD_handle.proxy_port);

    if (HYD_handle.debug)
        proxy_args[arg++] = HYDU_strdup("--debug");

    if (HYD_handle.enablex != -1) {
        proxy_args[arg++] = HYDU_strdup("--enable-x");
        proxy_args[arg++] = HYDU_int_to_str(HYD_handle.enablex);
    }

    proxy_args[arg++] = HYDU_strdup("--bootstrap");
    proxy_args[arg++] = HYDU_strdup(HYD_handle.bootstrap);

    if (HYD_handle.bootstrap_exec) {
        proxy_args[arg++] = HYDU_strdup("--bootstrap-exec");
        proxy_args[arg++] = HYDU_strdup(HYD_handle.bootstrap_exec);
    }

    proxy_args[arg++] = NULL;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status fill_in_exec_args(void)
{
    int i, arg, process_id;
    int inherited_env_count, user_env_count, system_env_count;
    int segment_count, exec_count, total_args;
    static int proxy_count = 0;
    HYD_Env_t *env;
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    struct HYD_Partition_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    /* Create the arguments list for each proxy */
    process_id = 0;
    FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list) {
        for (inherited_env_count = 0, env = HYD_handle.global_env.inherited; env;
             env = env->next, inherited_env_count++);
        for (user_env_count = 0, env = HYD_handle.global_env.user; env;
             env = env->next, user_env_count++);
        for (system_env_count = 0, env = HYD_handle.global_env.system; env;
             env = env->next, system_env_count++);

        for (segment_count = 0, segment = partition->segment_list; segment;
             segment = segment->next)
            segment_count++;

        for (exec_count = 0, exec = partition->exec_list; exec; exec = exec->next)
            exec_count++;

        total_args = HYD_NUM_TMP_STRINGS; /* For the basic arguments */

        /* Environments */
        total_args += inherited_env_count;
        total_args += user_env_count;
        total_args += system_env_count;

        /* For each segment add a few strings */
        total_args += (segment_count * HYD_NUM_TMP_STRINGS);

        /* For each exec add a few strings */
        total_args += (exec_count * HYD_NUM_TMP_STRINGS);

        HYDU_MALLOC(partition->base->exec_args, char **, total_args * sizeof(char *),
                    status);

        arg = 0;
        partition->base->exec_args[arg++] = HYDU_strdup("--global-core-count");
        partition->base->exec_args[arg++] = HYDU_int_to_str(HYD_handle.global_core_count);

        partition->base->exec_args[arg++] = HYDU_strdup("--wdir");
        partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.wdir);

        partition->base->exec_args[arg++] = HYDU_strdup("--pmi-port-str");
        if (HYD_handle.pm_env)
            partition->base->exec_args[arg++] = HYDU_strdup(pmi_port_str);
        else
            partition->base->exec_args[arg++] = HYDU_strdup("HYDRA_NULL");

        partition->base->exec_args[arg++] = HYDU_strdup("--binding");
        if (HYD_handle.binding)
            partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.binding);
        else
            partition->base->exec_args[arg++] = HYDU_strdup("HYDRA_NULL");
        if (HYD_handle.user_bind_map)
            partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.user_bind_map);
        else if (partition->user_bind_map)
            partition->base->exec_args[arg++] = HYDU_strdup(partition->user_bind_map);
        else
            partition->base->exec_args[arg++] = HYDU_strdup("HYDRA_NULL");

        partition->base->exec_args[arg++] = HYDU_strdup("--bindlib");
        partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.bindlib);

        partition->base->exec_args[arg++] = HYDU_strdup("--ckpointlib");
        partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.ckpointlib);

        partition->base->exec_args[arg++] = HYDU_strdup("--ckpoint-prefix");
        if (HYD_handle.ckpoint_prefix)
            partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.ckpoint_prefix);
        else
            partition->base->exec_args[arg++] = HYDU_strdup("HYDRA_NULL");

        if (HYD_handle.ckpoint_restart)
            partition->base->exec_args[arg++] = HYDU_strdup("--ckpoint-restart");

        partition->base->exec_args[arg++] = HYDU_strdup("--global-inherited-env");
        for (i = 0, env = HYD_handle.global_env.inherited; env; env = env->next, i++);
        partition->base->exec_args[arg++] = HYDU_int_to_str(i);
        partition->base->exec_args[arg++] = NULL;
        HYDU_list_append_env_to_str(HYD_handle.global_env.inherited,
                                    partition->base->exec_args);

        arg = HYDU_strlist_lastidx(partition->base->exec_args);
        partition->base->exec_args[arg++] = HYDU_strdup("--global-user-env");
        for (i = 0, env = HYD_handle.global_env.user; env; env = env->next, i++);
        partition->base->exec_args[arg++] = HYDU_int_to_str(i);
        partition->base->exec_args[arg++] = NULL;
        HYDU_list_append_env_to_str(HYD_handle.global_env.user,
                                    partition->base->exec_args);

        arg = HYDU_strlist_lastidx(partition->base->exec_args);
        partition->base->exec_args[arg++] = HYDU_strdup("--global-system-env");
        for (i = 0, env = HYD_handle.global_env.system; env; env = env->next, i++);
        partition->base->exec_args[arg++] = HYDU_int_to_str(i);
        partition->base->exec_args[arg++] = NULL;
        HYDU_list_append_env_to_str(HYD_handle.global_env.system,
                                    partition->base->exec_args);

        arg = HYDU_strlist_lastidx(partition->base->exec_args);
        partition->base->exec_args[arg++] = HYDU_strdup("--genv-prop");
        partition->base->exec_args[arg++] = HYDU_strdup(HYD_handle.global_env.prop);
        partition->base->exec_args[arg++] = NULL;

        /* Pass the segment information */
        for (segment = partition->segment_list; segment; segment = segment->next) {
            arg = HYDU_strlist_lastidx(partition->base->exec_args);
            partition->base->exec_args[arg++] = HYDU_strdup("--segment");

            partition->base->exec_args[arg++] = HYDU_strdup("--segment-start-pid");
            partition->base->exec_args[arg++] = HYDU_int_to_str(segment->start_pid);

            partition->base->exec_args[arg++] = HYDU_strdup("--segment-proc-count");
            partition->base->exec_args[arg++] = HYDU_int_to_str(segment->proc_count);
            partition->base->exec_args[arg++] = NULL;
        }

        /* Now pass the local executable information */
        for (exec = partition->exec_list; exec; exec = exec->next) {
            arg = HYDU_strlist_lastidx(partition->base->exec_args);
            partition->base->exec_args[arg++] = HYDU_strdup("--exec");

            partition->base->exec_args[arg++] = HYDU_strdup("--exec-proc-count");
            partition->base->exec_args[arg++] = HYDU_int_to_str(exec->proc_count);

            partition->base->exec_args[arg++] = HYDU_strdup("--exec-local-env");
            for (i = 0, env = exec->user_env; env; env = env->next, i++);
            partition->base->exec_args[arg++] = HYDU_int_to_str(i);
            partition->base->exec_args[arg++] = NULL;
            HYDU_list_append_env_to_str(exec->user_env, partition->base->exec_args);

            arg = HYDU_strlist_lastidx(partition->base->exec_args);
            partition->base->exec_args[arg++] = HYDU_strdup("--exec-env-prop");
            partition->base->exec_args[arg++] = exec->env_prop ? HYDU_strdup(exec->env_prop) :
                HYDU_strdup("HYDRA_NULL");
            partition->base->exec_args[arg++] = NULL;

            HYDU_list_append_strlist(exec->exec, partition->base->exec_args);

            process_id += exec->proc_count;
        }

        if (HYD_handle.debug) {
            printf("Arguments being passed to proxy %d:\n", proxy_count++);
            HYDU_print_strlist(partition->base->exec_args);
            printf("\n");
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_PMCI_launch_procs(void)
{
    struct HYD_Partition *partition;
    enum HYD_PMCD_pmi_proxy_cmds cmd;
    int fd, len, id;
#if defined HAVE_THREAD_SUPPORT
    struct HYD_Thread_context *thread_context = NULL;
#endif /* HAVE_THREAD_SUPPORT */
    char *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL };
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_set_common_signals(HYD_PMCD_pmi_serv_signal_cb);
    HYDU_ERR_POP(status, "unable to set signal\n");

    /* Initialize PMI */
    status = create_and_listen_portstr(HYD_PMCD_pmi_connect_cb, &pmi_port_str);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_handle.debug)
        HYDU_dump(stdout, "Got a PMI port string of %s\n", pmi_port_str);

    status = HYD_PMCD_pmi_init();
    HYDU_ERR_POP(status, "unable to create process group\n");

    if (HYD_handle.launch_mode == HYD_LAUNCH_RUNTIME) {
        status = create_and_listen_portstr(HYD_PMCD_pmi_serv_control_connect_cb,
                                           &proxy_port_str);
        HYDU_ERR_POP(status, "unable to create PMI port\n");
        if (HYD_handle.debug)
            HYDU_dump(stdout, "Got a proxy port string of %s\n", proxy_port_str);

        status = fill_in_proxy_args(HYD_handle.launch_mode, proxy_args);
        HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

        status = fill_in_exec_args();
        HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

        status = HYD_BSCI_launch_procs(proxy_args, "--partition-id", HYD_handle.partition_list);
        HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");
    }
    else if (HYD_handle.launch_mode == HYD_LAUNCH_BOOT ||
             HYD_handle.launch_mode == HYD_LAUNCH_BOOT_FOREGROUND) {
        status = fill_in_proxy_args(HYD_handle.launch_mode, proxy_args);
        HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

        status = HYD_BSCI_launch_procs(proxy_args, "--partition-id", HYD_handle.partition_list);
        HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");
    }
    else if (HYD_handle.launch_mode == HYD_LAUNCH_SHUTDOWN) {
        FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list) {
            status = HYDU_sock_connect(partition->base->name, HYD_handle.proxy_port, &fd);
            if (status != HYD_SUCCESS) {
                /* Don't abort. Try to shutdown as many proxies as possible */
                HYDU_Error_printf("Unable to connect to proxy at %s\n", partition->base->name);
                continue;
            }

            cmd = PROXY_SHUTDOWN;
            status = HYDU_sock_write(fd, &cmd, sizeof(enum HYD_PMCD_pmi_proxy_cmds));
            HYDU_ERR_POP(status, "unable to write data to proxy\n");

            close(fd);
        }
    }
    else if (HYD_handle.launch_mode == HYD_LAUNCH_PERSISTENT) {
        status = fill_in_exec_args();
        HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");

        len = 0;
        FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list)
            len++;

#if defined HAVE_THREAD_SUPPORT
        HYDU_CALLOC(thread_context, struct HYD_Thread_context *, len,
                    sizeof(struct HYD_Thread_context), status);
        if (!thread_context)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "Unable to allocate memory for thread context\n");
#endif /* HAVE_THREAD_SUPPORT */

        id = 0;
        FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list) {
#if defined HAVE_THREAD_SUPPORT
            HYDU_create_thread(launch_helper, (void *) partition, &thread_context[id]);
#else
            launch_helper(partition);
#endif /* HAVE_THREAD_SUPPORT */
            id++;
        }

        id = 0;
        FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list) {
#if defined HAVE_THREAD_SUPPORT
            HYDU_join_thread(thread_context[id]);
#endif /* HAVE_THREAD_SUPPORT */

            status = HYD_DMX_register_fd(1, &partition->control_fd, HYD_STDOUT, partition,
                                         HYD_PMCD_pmi_serv_control_cb);
            HYDU_ERR_POP(status, "unable to register control fd\n");

            id++;
        }
    }

  fn_exit:
    if (pmi_port_str)
        HYDU_FREE(pmi_port_str);
    if (proxy_port_str)
        HYDU_FREE(proxy_port_str);
#if defined HAVE_THREAD_SUPPORT
    if (thread_context)
        HYDU_FREE(thread_context);
#endif /* HAVE_THREAD_SUPPORT */
    HYDU_free_strlist(proxy_args);
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

    if ((HYD_handle.launch_mode == HYD_LAUNCH_BOOT) ||
        (HYD_handle.launch_mode == HYD_LAUNCH_SHUTDOWN)) {
        status = HYD_SUCCESS;
    }
    else {
        while (1) {
            /* Wait for some event to occur */
            status = HYD_DMX_wait_for_event(HYDU_time_left(HYD_handle.start, HYD_handle.timeout));
            HYDU_ERR_POP(status, "error waiting for event\n");

            /* If the timeout expired, raise a SIGINT and kill all the
             * processes */
            if (HYDU_time_left(HYD_handle.start, HYD_handle.timeout) == 0)
                raise(SIGINT);

            /* Check to see if there's any open read socket left; if
             * there are, we will just wait for more events. */
            sockets_open = 0;
            FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list) {
                if (partition->base->out != -1 || partition->base->err != -1) {
                    sockets_open++;
                    break;
                }
            }

            if (sockets_open && HYDU_time_left(HYD_handle.start, HYD_handle.timeout))
                continue;

            break;
        }

        do {
            /* Check if the exit status has already arrived */
            all_procs_exited = 1;
            FORALL_ACTIVE_PARTITIONS(partition, HYD_handle.partition_list) {
                if (partition->exit_status == NULL) {
                    all_procs_exited = 0;
                    break;
                }
            }

            if (all_procs_exited)
                break;

            /* If not, wait for some event to occur */
            status = HYD_DMX_wait_for_event(HYDU_time_left(HYD_handle.start, HYD_handle.timeout));
            HYDU_ERR_POP(status, "error waiting for event\n");
        } while (1);
    }

    status = HYD_BSCI_wait_for_completion(HYD_handle.partition_list);
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
