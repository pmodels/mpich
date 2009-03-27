/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmi_proxy.h"
#include "pmi_serv.h"
#include "demux.h"
#include "hydra_utils.h"

struct HYD_PMCD_pmi_proxy_params HYD_PMCD_pmi_proxy_params;

HYD_Status HYD_PMCD_pmi_proxy_get_next_keyvalp(char **bufp, int *buf_lenp, char **keyp,
                                               int *key_lenp, char **valp, int *val_lenp,
                                               char separator)
{
    char *p = NULL;
    int len = 0;
    int klen = 0;
    int vlen = 0;

    HYD_Status status = HYD_SUCCESS;

    p = *bufp;
    len = *buf_lenp;

    while (len && isspace(*p)) {
        p++;
        len--;
    }
    if (len <= 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "Error reading keyval from command\n");

    *keyp = p;
    klen = 0;
    while (len && (*p != '=')) {
        p++;
        len--;
        klen++;
    }
    if (key_lenp)
        *key_lenp = klen;
    p++;
    len--;
    if (len <= 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "Error reading keyval from command\n");

    *valp = p;
    vlen = 0;
    /* FIXME: Allow escaping ';' */
    while (len && (*p != separator)) {
        p++;
        len--;
        vlen++;
    }
    if (val_lenp)
        *val_lenp = vlen;
    p++;
    len--;
    if (len < 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "Error reading keyval from command\n");

    while (len && isspace(*p)) {
        p++;
        len--;
    }
    /* p now points to the next key or the end of string */
    *bufp = p;
    if (*p != '\0') {
        /* Remaining length of buffer to be processed */
        *buf_lenp = len;
    }
    else {
        /* End of string - no more keyvals */
        *buf_lenp = 0;
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_PMCD_pmi_proxy_handle_launch_cmd(int job_connfd, char *launch_cmd, int cmd_len)
{
    char *key, *val;
    int i = 0, key_len = 0, val_len = 0, core = 0;
    struct HYD_Partition_exec *exec = NULL;
    HYD_Env_t *env = NULL;
    HYD_Status status = HYD_SUCCESS;

    /* FIXME: We currently support only one job - We need a list of proxy params for multiple jobs */
    status = HYD_PMCD_pmi_proxy_init_params(&HYD_PMCD_pmi_proxy_params);
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

    HYD_PMCD_pmi_proxy_params.rproxy_connfd = job_connfd;

    status = HYD_DMX_deregister_fd(job_connfd);
    HYDU_ERR_POP(status, "Unable to deregister job conn fd\n");
    status =
        HYD_DMX_register_fd(1, &job_connfd, HYD_STDIN, (void *) &HYD_PMCD_pmi_proxy_params,
                            HYD_PMCD_pmi_proxy_remote_cb);
    HYDU_ERR_POP(status, "Unable to register job conn fd\n");

    status = HYDU_alloc_partition_exec(&HYD_PMCD_pmi_proxy_params.exec_list);
    HYDU_ERR_POP(status, "unable to allocate partition exec\n");

    exec = HYD_PMCD_pmi_proxy_params.exec_list;

    while (cmd_len > 0) {
        status =
            HYD_PMCD_pmi_proxy_get_next_keyvalp(&launch_cmd, &cmd_len, &key, &key_len, &val,
                                                &val_len, HYD_PMCD_CMD_SEP_CHAR);
        HYDU_ERR_POP(status, "Unable to get next key from launch command\n");

        /* FIXME: Use pre-defined macros for keys */
        if (!strncmp(key, "exec_name", key_len)) {
            HYDU_MALLOC(exec->exec[0], char *, (val_len + 1), status);
            HYDU_ERR_POP(status, "Error allocating memory for executable name\n");
            HYDU_snprintf(exec->exec[0], val_len, "%s", val);
            exec->exec[1] = NULL;
        }
        else if (!strncmp(key, "exec_cnt", key_len)) {
            for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec->next; exec = exec->next);
            exec->proc_count = atoi(val);
        }
        else if (!strncmp(key, "env", key_len)) {
            char *env_str;
            int env_str_len;

            env_str = val;
            env_str_len = val_len;
            exec->prop_env = NULL;
            while (env_str_len > 0) {
                status =
                    HYD_PMCD_pmi_proxy_get_next_keyvalp(&env_str, &env_str_len, &key, &key_len,
                                                        &val, &val_len,
                                                        HYD_PMCD_CMD_ENV_SEP_CHAR);
                HYDU_ERR_POP(status,
                             "Error getting next environment variable from launch command\n");

                HYDU_MALLOC(env, HYD_Env_t *, sizeof(HYD_Env_t), status);
                HYDU_ERR_POP(status,
                             "Error allocating memory for environment variable in proxy params\n");

                HYDU_MALLOC(env->env_name, char *, key_len + 1, status);
                HYDU_ERR_POP(status,
                             "Error allocating memory for environment variable in proxy params\n");
                HYDU_snprintf(env->env_name, key_len + 1, "%s", key);

                HYDU_MALLOC(env->env_value, char *, val_len + 1, status);
                HYDU_ERR_POP(status,
                             "Error allocating memory for environment variable in proxy params\n");
                HYDU_snprintf(env->env_value, val_len + 1, "%s", val);
                env->next = exec->prop_env;
                exec->prop_env = env;
            }
        }
        else {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "Unrecognized key in launch command\n");
        }

        /* FIXME: Set these ... */
        /* HYD_PMCD_pmi_proxy_params.wdir =
         * HYD_PMCD_pmi_proxy_params.binding =
         * HYD_PMCD_pmi_proxy_params.user_bind_map = ;
         * HYDU_append_env_to_list(*env, &HYD_PMCD_pmi_proxy_params.global_env);
         * HYD_PMCD_pmi_proxy_params.one_pass_count
         * status = HYDU_alloc_partition_segment(&segment->next);
         * segment->proc_count = ;
         * segment->start_pid = ;
         */
    }

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

    for (exec = HYD_PMCD_pmi_proxy_params.exec_list; exec; exec = exec->next) {
        for (i = 0; i < exec->proc_count; i++) {
            char *str = NULL;
            core = 0;
            env = NULL;
            /* FIXME: Use the start pmi_id from launch command */
            str = HYDU_int_to_str(i);
            status = HYDU_env_create(&env, "PMI_ID", str);
            HYDU_ERR_POP(status, "unable to create env\n");
            status = HYDU_putenv(env);
            HYDU_ERR_POP(status, "putenv failed\n");
            status = HYDU_create_process(&exec->exec[0], exec->prop_env, NULL,
                                         &HYD_PMCD_pmi_proxy_params.out[i],
                                         &HYD_PMCD_pmi_proxy_params.err[i],
                                         &HYD_PMCD_pmi_proxy_params.pid[i], core);
            HYDU_ERR_POP(status, "Error launching process\n");
            status =
                HYD_DMX_register_fd(1, &HYD_PMCD_pmi_proxy_params.out[i], HYD_STDOUT,
                                    (void *) &HYD_PMCD_pmi_proxy_params,
                                    HYD_PMCD_pmi_proxy_rstdout_cb);
            HYDU_ERR_POP(status, "Error registering process stdout\n");
        }
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/* Handle proxy commands */
HYD_Status HYD_PMCD_pmi_proxy_handle_cmd(int fd, char *cmd, int cmd_len)
{
    char *key = NULL;
    char *cmd_name = NULL;
    int i = 0, key_len = 0, cmd_name_len = 0;
    char *cmdp = NULL;
    HYD_Status status = HYD_SUCCESS;

    cmdp = cmd;
    /* First key/val is the command name */
    status = HYD_PMCD_pmi_proxy_get_next_keyvalp(&cmdp, &cmd_len, &key, &key_len, &cmd_name,
                                                 &cmd_name_len, HYD_PMCD_CMD_SEP_CHAR);
    HYDU_ERR_POP(status, "Error retreiving command name from command\n");

    if (!strncmp(cmd_name, HYD_PMCD_CMD_KILLALL_PROCS, key_len)) {
        for (i = 0; i < HYD_PMCD_pmi_proxy_params.exec_proc_count; i++)
            if (HYD_PMCD_pmi_proxy_params.pid[i] != -1)
                kill(HYD_PMCD_pmi_proxy_params.pid[i], SIGKILL);

        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to register fd\n");
        close(fd);
    }
    else if (!strncmp(cmd_name, HYD_PMCD_CMD_LAUNCH_PROCS, key_len)) {
        status = HYD_PMCD_pmi_proxy_handle_launch_cmd(fd, cmdp, cmd_len);
        HYDU_ERR_POP(status, "Unable to handle launch command\n");
    }
    else if (!strncmp(cmd_name, HYD_PMCD_CMD_SHUTDOWN, key_len)) {
        /* FIXME: Not a clean shutdown... Kill all procs before exiting */
        status = HYD_DMX_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to register fd\n");
        close(fd);
        exit(0);
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "got unrecognized command from mpiexec\n");
    }
  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

/* Initialize proxy params */
HYD_Status HYD_PMCD_pmi_proxy_init_params(struct HYD_PMCD_pmi_proxy_params *proxy_params)
{
    HYD_Status status = HYD_SUCCESS;
    proxy_params->debug = 0;
    proxy_params->proxy_port = -1;
    proxy_params->is_persistent = 0;
    proxy_params->wdir = NULL;
    proxy_params->binding = HYD_BIND_UNSET;
    proxy_params->user_bind_map = NULL;
    proxy_params->global_env = NULL;
    proxy_params->one_pass_count = 0;
    proxy_params->partition_proc_count = 0;
    proxy_params->exec_proc_count = 0;
    proxy_params->segment_list = NULL;
    proxy_params->exec_list = NULL;
    proxy_params->pid = NULL;
    proxy_params->out = NULL;
    proxy_params->err = NULL;
    proxy_params->exit_status = NULL;
    proxy_params->in = -1;
    proxy_params->rproxy_connfd = -1;
    proxy_params->stdin_buf_offset = 0;
    proxy_params->stdin_buf_count = 0;
    proxy_params->stdin_tmp_buf[0] = '\0';
    return status;
}

/* Cleanup proxy params after use */
HYD_Status HYD_PMCD_pmi_proxy_cleanup_params(struct HYD_PMCD_pmi_proxy_params * proxy_params)
{
    HYD_Status status = HYD_SUCCESS;
    if (proxy_params->wdir != NULL)
        HYDU_FREE(proxy_params->wdir);
    if (proxy_params->user_bind_map != NULL)
        HYDU_FREE(proxy_params->user_bind_map);
    if (proxy_params->global_env != NULL) {
        HYD_Env_t *p, *q;
        do {
            p = proxy_params->global_env;
            q = p->next;
            HYDU_FREE(p);
        } while (q);
    }
    if (proxy_params->segment_list != NULL) {
        /* FIXME : incomplete */
    }
    if (proxy_params->exec_list != NULL) {
        struct HYD_Partition_exec *p, *q;
        do {
            p = proxy_params->exec_list;
            q = p->next;
            HYDU_FREE(p);
        } while (q);
    }
    if (proxy_params->pid != NULL)
        HYDU_FREE(proxy_params->pid);
    if (proxy_params->out != NULL)
        HYDU_FREE(proxy_params->out);
    if (proxy_params->err != NULL)
        HYDU_FREE(proxy_params->err);
    if (proxy_params->exit_status != NULL)
        HYDU_FREE(proxy_params->exit_status);

    status = HYD_PMCD_pmi_proxy_init_params(proxy_params);
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_PMCD_pmi_proxy_get_params(int t_argc, char **t_argv)
{
    char **argv = t_argv, *str;
    int arg, i, count;
    HYD_Env_t *env;
    struct HYD_Partition_exec *exec = NULL;
    struct HYD_Partition_segment *segment = NULL;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_PMCD_pmi_proxy_init_params(&HYD_PMCD_pmi_proxy_params);
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

    while (*argv) {
        ++argv;
        if (*argv == NULL)
            break;

        if (!strcmp(*argv, "--verbose")) {
            HYD_PMCD_pmi_proxy_params.debug = 1;
            continue;
        }
        /* Proxy port */
        if (!strcmp(*argv, "--proxy-port")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.proxy_port = atoi(*argv);
            continue;
        }

        if (!strcmp(*argv, "--persistent")) {
            HYD_PMCD_pmi_proxy_params.is_persistent = 1;
            continue;
        }

        /* Working directory */
        if (!strcmp(*argv, "--wdir")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.wdir = HYDU_strdup(*argv);
            continue;
        }

        /* Working directory */
        if (!strcmp(*argv, "--binding")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.binding = atoi(*argv);
            argv++;
            if (!strcmp(*argv, "HYDRA_NO_USER_MAP"))
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
        if (!strcmp(*argv, "--one-pass-count")) {
            argv++;
            HYD_PMCD_pmi_proxy_params.one_pass_count = atoi(*argv);
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
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
