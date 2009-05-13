/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmi_proxy.h"
#include "demux.h"
#include "hydra_utils.h"

struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;

static HYD_Status init_params(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYD_PMCD_pmi_proxy_params.debug = 0;

    HYD_PMCD_pmi_proxy_params.proxy.server_name = NULL;
    HYD_PMCD_pmi_proxy_params.proxy.server_port = -1;
    HYD_PMCD_pmi_proxy_params.proxy.launch_mode = HYD_LAUNCH_UNSET;
    HYD_PMCD_pmi_proxy_params.proxy.partition_id = -1;

    HYD_PMCD_pmi_proxy_params.bootstrap = NULL;
    HYD_PMCD_pmi_proxy_params.wdir = NULL;
    HYD_PMCD_pmi_proxy_params.pmi_port_str = NULL;
    HYD_PMCD_pmi_proxy_params.binding = HYD_BIND_UNSET;
    HYD_PMCD_pmi_proxy_params.user_bind_map = NULL;

    HYD_PMCD_pmi_proxy_params.global_env = NULL;

    HYD_PMCD_pmi_proxy_params.global_core_count = 0;
    HYD_PMCD_pmi_proxy_params.partition_core_count = 0;
    HYD_PMCD_pmi_proxy_params.exec_proc_count = 0;

    HYD_PMCD_pmi_proxy_params.procs_are_launched = 0;

    HYD_PMCD_pmi_proxy_params.segment_list = NULL;
    HYD_PMCD_pmi_proxy_params.exec_list = NULL;

    HYD_PMCD_pmi_proxy_params.upstream.out = -1;
    HYD_PMCD_pmi_proxy_params.upstream.err = -1;
    HYD_PMCD_pmi_proxy_params.upstream.in = -1;
    HYD_PMCD_pmi_proxy_params.upstream.control = -1;

    HYD_PMCD_pmi_proxy_params.pid = NULL;
    HYD_PMCD_pmi_proxy_params.out = NULL;
    HYD_PMCD_pmi_proxy_params.err = NULL;
    HYD_PMCD_pmi_proxy_params.exit_status = NULL;
    HYD_PMCD_pmi_proxy_params.in = -1;

    HYD_PMCD_pmi_proxy_params.stdin_buf_offset = 0;
    HYD_PMCD_pmi_proxy_params.stdin_buf_count = 0;
    HYD_PMCD_pmi_proxy_params.stdin_tmp_buf[0] = '\0';

    return status;
}

/* FIXME: This function performs minimal error checking as it is not
 * supposed to be called by the user, but rather by the process
 * management server. It will still be helpful for debugging to add
 * some error checks. */
static HYD_Status parse_params(char **t_argv)
{
    char **argv = t_argv, *str;
    int arg, i, count;
    HYD_Env_t *env;
    struct HYD_Partition_exec *exec = NULL;
    struct HYD_Partition_segment *segment = NULL;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (; *argv; ++argv) {
        /* Working directory */
        if (!strcmp(*argv, "--wdir")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.wdir = HYDU_strdup(*argv);
            continue;
        }

        /* PMI port string */
        if (!strcmp(*argv, "--pmi-port-str")) {
            argv++;
            if (!strcmp(*argv, "HYDRA_NULL"))
                HYD_PMCD_pmi_proxy_params.pmi_port_str = NULL;
            else
                HYD_PMCD_pmi_proxy_params.pmi_port_str = HYDU_strdup(*argv);
            continue;
        }

        /* Working directory */
        if (!strcmp(*argv, "--binding")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.binding = atoi(*argv);
            argv++;
            if (!strcmp(*argv, "HYDRA_NULL"))
                HYD_PMCD_pmi_proxy_params.user_bind_map = NULL;
            else
                HYD_PMCD_pmi_proxy_params.user_bind_map = HYDU_strdup(*argv);
            continue;
        }

        /* Global env */
        if (!strcmp(*argv, "--global-env")) {
            argv++;
            count = atoi(*argv);
            for (i = 0; i < count; i++) {
                argv++;
                str = *argv;

                /* Some bootstrap servers remove the quotes that we
                 * added, while some others do not. For the cases
                 * where they are not removed, we do it ourselves. */
                if (*str == '\'') {
                    str++;
                    str[strlen(str) - 1] = 0;
                }
                env = HYDU_str_to_env(str);
                HYDU_append_env_to_list(*env, &HYD_PMCD_pmi_proxy_params.global_env);
                HYDU_FREE(env);
            }
            continue;
        }

        /* One-pass Count */
        if (!strcmp(*argv, "--global-core-count")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.global_core_count = atoi(*argv);
            continue;
        }

        /* New segment */
        if (!strcmp(*argv, "--segment")) {
            if (HYD_PMCD_pmi_proxy_params.segment_list == NULL) {
                status = HYDU_alloc_partition_segment(&HYD_PMCD_pmi_proxy_params.segment_list);
                HYDU_ERR_POP(status, "unable to allocate partition segment\n");
            }
            else {
                for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment->next;
                     segment = segment->next);
                status = HYDU_alloc_partition_segment(&segment->next);
                HYDU_ERR_POP(status, "unable to allocate partition segment\n");
            }
            continue;
        }

        /* Process count */
        if (!strcmp(*argv, "--segment-proc-count")) {
            argv++;
            for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment->next;
                 segment = segment->next);
            segment->proc_count = atoi(*argv);
            continue;
        }

        /* Process count */
        if (!strcmp(*argv, "--segment-start-pid")) {
            argv++;
            for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment->next;
                 segment = segment->next);
            segment->start_pid = atoi(*argv);
            continue;
        }

        /* New executable */
        if (!strcmp(*argv, "--exec")) {
            if (HYD_PMCD_pmi_proxy_params.exec_list == NULL) {
                status = HYDU_alloc_partition_exec(&HYD_PMCD_pmi_proxy_params.exec_list);
                HYDU_ERR_POP(status, "unable to allocate partition exec\n");
            }
            else {
                for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next;
                     exec = exec->next);
                status = HYDU_alloc_partition_exec(&exec->next);
                HYDU_ERR_POP(status, "unable to allocate partition exec\n");
            }
            continue;
        }

        /* Process count */
        if (!strcmp(*argv, "--exec-proc-count")) {
            argv++;
            for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next; exec = exec->next);
            exec->proc_count = atoi(*argv);
            continue;
        }

        /* Local env */
        if (!strcmp(*argv, "--exec-local-env")) {
            argv++;
            count = atoi(*argv);
            for (i = 0; i < count; i++) {
                argv++;
                str = *argv;

                /* Some bootstrap servers remove the quotes that we
                 * added, while some others do not. For the cases
                 * where they are not removed, we do it ourselves. */
                if (*str == '\'') {
                    str++;
                    str[strlen(str) - 1] = 0;
                }
                env = HYDU_str_to_env(str);
                HYDU_append_env_to_list(*env, &exec->prop_env);
                HYDU_FREE(env);
            }
            continue;
        }

        /* Fall through case is application parameters. Load
         * everything into the args variable. */
        for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next; exec = exec->next);
        for (arg = 0; *argv && strcmp(*argv, "--exec");) {
            exec->exec[arg++] = HYDU_strdup(*argv);
            ++argv;
        }
        exec->exec[arg++] = NULL;

        /* If we already touched the next --exec, step back */
        if (*argv && !strcmp(*argv, "--exec"))
            argv--;

        if (!(*argv))
            break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_get_params(char **t_argv)
{
    char **argv = t_argv;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = init_params();
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

    while (++argv && *argv) {
        if (!strcmp(*argv, "--launch-mode")) {
            ++argv;
            HYD_PMCD_pmi_proxy_params.proxy.launch_mode = atoi(*argv);
            continue;
        }
        if (!strcmp(*argv, "--proxy-port")) {
            ++argv;
            if (HYD_PMCD_pmi_proxy_params.proxy.launch_mode == HYD_LAUNCH_RUNTIME) {
                HYD_PMCD_pmi_proxy_params.proxy.server_name = HYDU_strdup(strtok(*argv, ":"));
                HYD_PMCD_pmi_proxy_params.proxy.server_port = atoi(strtok(NULL, ":"));
            }
            else {
                HYD_PMCD_pmi_proxy_params.proxy.server_port = atoi(*argv);
            }
            continue;
        }
        if (!strcmp(*argv, "--partition-id")) {
            ++argv;
            HYD_PMCD_pmi_proxy_params.proxy.partition_id = atoi(*argv);
            continue;
        }
        if (!strcmp(*argv, "--debug")) {
            HYD_PMCD_pmi_proxy_params.debug = 1;
            continue;
        }
        if (!strcmp(*argv, "--bootstrap")) {
            ++argv;
            HYD_PMCD_pmi_proxy_params.bootstrap = HYDU_strdup(*argv);
            continue;
        }
    }

    status = HYD_BSCI_init(HYD_PMCD_pmi_proxy_params.bootstrap,
                           HYD_PMCD_pmi_proxy_params.debug);
    HYDU_ERR_POP(status, "proxy unable to initialize bootstrap\n");

    if (HYD_PMCD_pmi_proxy_params.proxy.partition_id == -1) {
        /* We didn't get a partition ID during launch; query the
         * bootstrap server for it. */
        status = HYD_BSCI_query_partition_id(&HYD_PMCD_pmi_proxy_params.proxy.partition_id);
        HYDU_ERR_POP(status, "unable to query bootstrap server for partition ID\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_cleanup_params(void)
{
    struct HYD_Partition_segment *segment, *tsegment;
    struct HYD_Partition_exec *exec, *texec;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_PMCD_pmi_proxy_params.proxy.server_name)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.proxy.server_name);

    if (HYD_PMCD_pmi_proxy_params.bootstrap)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.bootstrap);

    if (HYD_PMCD_pmi_proxy_params.wdir)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.wdir);

    if (HYD_PMCD_pmi_proxy_params.pmi_port_str)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.pmi_port_str);

    if (HYD_PMCD_pmi_proxy_params.user_bind_map)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.user_bind_map);

    if (HYD_PMCD_pmi_proxy_params.global_env)
        HYDU_env_free_list(HYD_PMCD_pmi_proxy_params.global_env);

    if (HYD_PMCD_pmi_proxy_params.segment_list) {
        segment = HYD_PMCD_pmi_proxy_params.segment_list;
        while (segment) {
            tsegment = segment->next;
            if (segment->mapping) {
                HYDU_free_strlist(segment->mapping);
                HYDU_FREE(segment->mapping);
            }
            HYDU_FREE(segment);
            segment = tsegment;
        }
    }

    if (HYD_PMCD_pmi_proxy_params.exec_list) {
        exec = HYD_PMCD_pmi_proxy_params.exec_list;
        while (exec) {
            texec = exec->next;
            HYDU_free_strlist(exec->exec);
            if (exec->prop_env)
                HYDU_env_free(exec->prop_env);
            HYDU_FREE(exec);
            exec = texec;
        }
    }

    if (HYD_PMCD_pmi_proxy_params.pid)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.pid);

    if (HYD_PMCD_pmi_proxy_params.out)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.out);

    if (HYD_PMCD_pmi_proxy_params.err)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.err);

    if (HYD_PMCD_pmi_proxy_params.exit_status)
        HYDU_FREE(HYD_PMCD_pmi_proxy_params.exit_status);

    /* Reinitialize all params to set everything to "NULL" or
     * equivalent. */
    status = init_params();
    HYDU_ERR_POP(status, "unable to initialize params\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_procinfo(int fd)
{
    char **arglist;
    int num_strings, str_len, recvd, i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Read information about the application to launch into a string
     * array and call parse_params() to interpret it and load it into
     * the proxy handle. */
    status = HYDU_sock_read(fd, &num_strings, sizeof(int), &recvd, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading data from upstream\n");

    HYDU_MALLOC(arglist, char **, (num_strings + 1) * sizeof(char *), status);

    for (i = 0; i < num_strings; i++) {
        status = HYDU_sock_read(fd, &str_len, sizeof(int), &recvd, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");

        HYDU_MALLOC(arglist[i], char *, str_len, status);

        status = HYDU_sock_read(fd, arglist[i], str_len, &recvd, HYDU_SOCK_COMM_MSGWAIT);
        HYDU_ERR_POP(status, "error reading data from upstream\n");
    }
    arglist[num_strings] = NULL;

    /* Get the parser to fill in the proxy params structure. */
    status = parse_params(arglist);
    HYDU_ERR_POP(status, "unable to parse argument list\n");

    HYDU_free_strlist(arglist);
    HYDU_FREE(arglist);

    /* Save this fd as we need to send back the exit status on
     * this. */
    HYD_PMCD_pmi_proxy_params.upstream.control = fd;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_proxy_launch_procs(void)
{
    int i, j, arg, stdin_fd, process_id, core, pmi_id;
    char *str;
    char *client_args[HYD_NUM_TMP_STRINGS];
    HYD_Env_t *env;
    struct HYD_Partition_segment *segment;
    struct HYD_Partition_exec *exec;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_PMCD_pmi_proxy_params.partition_core_count = 0;
    for (segment = HYD_PMCD_pmi_proxy_params.segment_list; segment; segment = segment->next)
        HYD_PMCD_pmi_proxy_params.partition_core_count += segment->proc_count;

    HYD_PMCD_pmi_proxy_params.exec_proc_count = 0;
    for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec; exec = exec->next)
        HYD_PMCD_pmi_proxy_params.exec_proc_count += exec->proc_count;

    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.out, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.err, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.pid, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);
    HYDU_MALLOC(HYD_PMCD_pmi_proxy_params.exit_status, int *,
                HYD_PMCD_pmi_proxy_params.exec_proc_count * sizeof(int), status);

    /* Initialize the exit status */
    for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
        HYD_PMCD_pmi_proxy_params.exit_status[i] = -1;

    /* For local spawning, set the global environment here itself */
    status = HYDU_putenv_list(HYD_PMCD_pmi_proxy_params.global_env);
    HYDU_ERR_POP(status, "putenv returned error\n");

    status = HYDU_bind_init(HYD_PMCD_pmi_proxy_params.user_bind_map);
    HYDU_ERR_POP(status, "unable to initialize process binding\n");

    /* Set the PMI port string to connect to. We currently just use
     * the global PMI port. */
    if (HYD_PMCD_pmi_proxy_params.pmi_port_str) {
        str = HYDU_strdup(HYD_PMCD_pmi_proxy_params.pmi_port_str);
        status = HYDU_env_create(&env, "PMI_PORT", str);
        HYDU_ERR_POP(status, "unable to create env\n");
        HYDU_FREE(str);
        status = HYDU_putenv(env);
        HYDU_ERR_POP(status, "putenv failed\n");
    }

    /* Spawn the processes */
    process_id = 0;
    for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec; exec = exec->next) {
        for (i = 0; i < exec->proc_count; i++) {

            pmi_id = HYDU_local_to_global_id(process_id,
                                             HYD_PMCD_pmi_proxy_params.partition_core_count,
                                             HYD_PMCD_pmi_proxy_params.segment_list,
                                             HYD_PMCD_pmi_proxy_params.global_core_count);

            if (HYD_PMCD_pmi_proxy_params.pmi_port_str) {
                str = HYDU_int_to_str(pmi_id);
                status = HYDU_env_create(&env, "PMI_ID", str);
                HYDU_ERR_POP(status, "unable to create env\n");
                HYDU_FREE(str);
                status = HYDU_putenv(env);
                HYDU_ERR_POP(status, "putenv failed\n");
            }

            if (chdir(HYD_PMCD_pmi_proxy_params.wdir) < 0)
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                     "unable to change wdir (%s)\n", HYDU_strerror(errno));

            for (j = 0, arg = 0; exec->exec[j]; j++)
                client_args[arg++] = HYDU_strdup(exec->exec[j]);
            client_args[arg++] = NULL;

            core = HYDU_bind_get_core_id(process_id, HYD_PMCD_pmi_proxy_params.binding);
            if (pmi_id == 0) {
                status = HYDU_create_process(client_args, exec->prop_env,
                                             &HYD_PMCD_pmi_proxy_params.in,
                                             &HYD_PMCD_pmi_proxy_params.out[process_id],
                                             &HYD_PMCD_pmi_proxy_params.err[process_id],
                                             &HYD_PMCD_pmi_proxy_params.pid[process_id], core);

                HYD_PMCD_pmi_proxy_params.stdin_buf_offset = 0;
                HYD_PMCD_pmi_proxy_params.stdin_buf_count = 0;

                stdin_fd = HYD_PMCD_pmi_proxy_params.upstream.in;
                status = HYD_DMX_register_fd(1, &stdin_fd, HYD_STDIN, NULL,
                                             HYD_PMCD_pmi_proxy_stdin_cb);
                HYDU_ERR_POP(status, "unable to register fd\n");
            }
            else {
                status = HYDU_create_process(client_args, exec->prop_env,
                                             NULL,
                                             &HYD_PMCD_pmi_proxy_params.out[process_id],
                                             &HYD_PMCD_pmi_proxy_params.err[process_id],
                                             &HYD_PMCD_pmi_proxy_params.pid[process_id], core);
            }
            HYDU_ERR_POP(status, "spawn process returned error\n");

            process_id++;
        }
    }

    /* Everything is spawned, register the required FDs  */
    status = HYD_DMX_register_fd(HYD_PMCD_pmi_proxy_params.exec_proc_count,
                                 HYD_PMCD_pmi_proxy_params.out,
                                 HYD_STDOUT, NULL, HYD_PMCD_pmi_proxy_stdout_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYD_DMX_register_fd(HYD_PMCD_pmi_proxy_params.exec_proc_count,
                                 HYD_PMCD_pmi_proxy_params.err,
                                 HYD_STDOUT, NULL, HYD_PMCD_pmi_proxy_stderr_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    /* Indicate that the processes have been launched */
    HYD_PMCD_pmi_proxy_params.procs_are_launched = 1;

    HYDU_FUNC_EXIT();

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}


void HYD_PMCD_pmi_proxy_killjob(void)
{
    int i;

    HYDU_FUNC_ENTER();

    /* Send the kill signal to all processes */
    for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++) {
        if (HYD_PMCD_pmi_proxy_params.pid[i] != -1) {
            kill(HYD_PMCD_pmi_proxy_params.pid[i], SIGTERM);
            kill(HYD_PMCD_pmi_proxy_params.pid[i], SIGKILL);
        }
    }

    HYDU_FUNC_EXIT();
}
