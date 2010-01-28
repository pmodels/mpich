/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "pmip.h"
#include "pmip_pmi.h"
#include "ckpoint.h"
#include "demux.h"
#include "bind.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;
struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_handle = { 0 };

static HYD_status pmi_cb(int fd, HYD_event_t events, void *userp)
{
    char *buf = NULL, *pmi_cmd, *args[HYD_NUM_TMP_STRINGS];
    int closed;
    struct HYD_pmcd_pmi_cmd_hdr hdr;
    enum HYD_pmcd_pmi_cmd cmd;
    struct HYD_pmcd_pmip_pmi_handle *h;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_read_pmi_cmd(fd, &buf, &hdr.pmi_version, &closed);
    HYDU_ERR_POP(status, "unable to read PMI command\n");

    if (hdr.pmi_version == 1)
        HYD_pmcd_pmip_pmi_handle = HYD_pmcd_pmip_pmi_v1;
    else
        HYD_pmcd_pmip_pmi_handle = HYD_pmcd_pmip_pmi_v2;

    if (closed) {
        /* A process died; kill the remaining processes */
        HYD_pmcd_pmip_killjob();
        goto fn_exit;
    }

    status = HYD_pmcd_pmi_parse_pmi_cmd(buf, hdr.pmi_version, &pmi_cmd, args);
    HYDU_ERR_POP(status, "unable to parse PMI command\n");

    h = HYD_pmcd_pmip_pmi_handle;
    while (h->handler) {
        if (!strcmp(pmi_cmd, h->cmd)) {
            status = h->handler(fd, args);
            HYDU_ERR_POP(status, "PMI handler returned error\n");
            goto fn_exit;
        }
        h++;
    }

    /* We don't understand the command; forward it upstream */
    cmd = PMI_CMD;
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &cmd, sizeof(cmd));
    HYDU_ERR_POP(status, "unable to send PMI_CMD command\n");

    hdr.pid = fd;
    hdr.buflen = strlen(buf);
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr));
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");

  fn_exit:
    if (buf)
        HYDU_FREE(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status handle_pmi_response(int fd)
{
    struct HYD_pmcd_pmi_response_hdr hdr;
    int count;
    char *buf;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_read(fd, &hdr, sizeof(hdr), &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI header from proxy\n");

    HYDU_MALLOC(buf, char *, hdr.buflen + 1, status);

    status = HYDU_sock_read(fd, buf, hdr.buflen, &count, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to read PMI response from proxy\n");

    buf[hdr.buflen] = 0;

    status = HYDU_sock_write(hdr.pid, buf, hdr.buflen);
    HYDU_ERR_POP(status, "unable to forward PMI response to MPI process\n");

    if (hdr.finalize) {
        status = HYDT_dmx_deregister_fd(hdr.pid);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(hdr.pid);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_listen_cb(int fd, HYD_event_t events, void *userp)
{
    int accept_fd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_accept(fd, &accept_fd);
    HYDU_ERR_POP(status, "accept error\n");

    status = HYDT_dmx_register_fd(1, &accept_fd, HYD_POLLIN, userp, pmi_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status launch_procs(void)
{
    int i, j, arg, stdin_fd, process_id, os_index, pmi_id;
    char *str, *envstr, *list;
    char *client_args[HYD_NUM_TMP_STRINGS];
    struct HYD_env *env, *opt_env = NULL, *force_env = NULL;
    struct HYD_exec *exec;
    enum HYD_pmcd_pmi_cmd cmd;
    char *pmi_port = NULL;
    int *pmi_ids;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_pmip.local.proxy_process_count = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next)
        HYD_pmcd_pmip.local.proxy_process_count += exec->proc_count;

    HYDU_MALLOC(pmi_ids, int *, HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        pmi_ids[i] =
            HYDU_local_to_global_id(i, HYD_pmcd_pmip.start_pid,
                                    HYD_pmcd_pmip.local.proxy_core_count,
                                    HYD_pmcd_pmip.system_global.global_core_count);
    }

    HYDU_MALLOC(HYD_pmcd_pmip.downstream.out, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.err, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.pid, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);
    HYDU_MALLOC(HYD_pmcd_pmip.downstream.exit_status, int *,
                HYD_pmcd_pmip.local.proxy_process_count * sizeof(int), status);

    /* Initialize the exit status */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
        HYD_pmcd_pmip.downstream.exit_status[i] = -1;

    status = HYDT_bind_init(HYD_pmcd_pmip.local.local_binding ?
                            HYD_pmcd_pmip.local.local_binding :
                            HYD_pmcd_pmip.user_global.binding,
                            HYD_pmcd_pmip.user_global.bindlib);
    HYDU_ERR_POP(status, "unable to initialize process binding\n");

    status = HYDT_ckpoint_init(HYD_pmcd_pmip.user_global.ckpointlib,
                               HYD_pmcd_pmip.user_global.ckpoint_prefix);
    HYDU_ERR_POP(status, "unable to initialize checkpointing\n");

    if (HYD_pmcd_pmip.exec_list == NULL) {      /* Checkpoint restart cast */
        status = HYDU_env_create(&env, "PMI_PORT", HYD_pmcd_pmip.system_global.pmi_port);
        HYDU_ERR_POP(status, "unable to create env\n");

        /* Restart the proxy.  Specify stdin fd only if pmi_id 0 is in this proxy. */
        status = HYDT_ckpoint_restart(env, HYD_pmcd_pmip.local.proxy_process_count,
                                      pmi_ids,
                                      pmi_ids[0] ? NULL :
                                      HYD_pmcd_pmip.system_global.enable_stdin ?
                                      &HYD_pmcd_pmip.downstream.in : NULL,
                                      HYD_pmcd_pmip.downstream.out,
                                      HYD_pmcd_pmip.downstream.err);
        HYDU_ERR_POP(status, "checkpoint restart failure\n");
        goto fn_spawn_complete;
    }

    if (HYD_pmcd_pmip.system_global.pmi_port == NULL) {
        /* No global PMI port is specified, we use a local one */
        status = HYDU_sock_create_and_listen_portstr(NULL, &pmi_port, pmi_listen_cb, NULL);
        HYDU_ERR_POP(status, "unable to create PMI port\n");
    }

    /* Spawn the processes */
    process_id = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {

        /* Increasing priority order: (1) global inherited env; (2)
         * global user env; (3) local user env; (4) system env. We
         * just set them one after the other, overwriting the previous
         * written value if needed. */

        if (!exec->env_prop && HYD_pmcd_pmip.user_global.global_env.prop)
            exec->env_prop = HYDU_strdup(HYD_pmcd_pmip.user_global.global_env.prop);

        if (!exec->env_prop) {
            /* user didn't specify anything; add inherited env to optional env */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &opt_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (!strcmp(exec->env_prop, "all")) {
            /* user explicitly asked us to pass all the environment */
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if (!strncmp(exec->env_prop, "list", strlen("list"))) {
            if (exec->env_prop)
                list = HYDU_strdup(exec->env_prop + strlen("list:"));
            else
                list = HYDU_strdup(HYD_pmcd_pmip.user_global.global_env.prop +
                                   strlen("list:"));

            envstr = strtok(list, ",");
            while (envstr) {
                env = HYDU_env_lookup(envstr, HYD_pmcd_pmip.user_global.global_env.inherited);
                if (env) {
                    status = HYDU_append_env_to_list(*env, &force_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                }
                envstr = strtok(NULL, ",");
            }
        }

        /* global user env */
        for (env = HYD_pmcd_pmip.user_global.global_env.user; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* local user env */
        for (env = exec->user_env; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* system env */
        for (env = HYD_pmcd_pmip.user_global.global_env.system; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        if (HYD_pmcd_pmip.system_global.pmi_port) {
            /* If a global PMI port is provided, use it */
            status = HYDU_env_create(&env, "PMI_PORT", HYD_pmcd_pmip.system_global.pmi_port);
            HYDU_ERR_POP(status, "unable to create env\n");
        }
        else {
            status = HYDU_env_create(&env, "PMI_PORT", pmi_port);
            HYDU_ERR_POP(status, "unable to create env\n");
        }

        status = HYDU_append_env_to_list(*env, &force_env);
        HYDU_ERR_POP(status, "unable to add env to list\n");

        /* Set the interface hostname based on what the user provided */
        if (HYD_pmcd_pmip.local.interface_env_name) {
            if (HYD_pmcd_pmip.user_global.iface) {
                /* The user asked us to use a specific interface; let's find it */
                status = HYDU_env_create(&env, HYD_pmcd_pmip.local.interface_env_name,
                                         HYD_pmcd_pmip.user_global.iface);
                HYDU_ERR_POP(status, "unable to create env\n");

                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
            else if (HYD_pmcd_pmip.local.hostname) {
                /* The second choice is the hostname the user gave */
                status = HYDU_env_create(&env, HYD_pmcd_pmip.local.interface_env_name,
                                         HYD_pmcd_pmip.local.hostname);
                HYDU_ERR_POP(status, "unable to create env\n");

                status = HYDU_append_env_to_list(*env, &force_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }

        if (exec->wdir && chdir(exec->wdir) < 0)
            HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                                 "unable to change wdir to %s (%s)\n", exec->wdir,
                                 HYDU_strerror(errno));

        for (i = 0; i < exec->proc_count; i++) {
            if (HYD_pmcd_pmip.system_global.pmi_id == -1)
                pmi_id = HYDU_local_to_global_id(process_id,
                                                 HYD_pmcd_pmip.start_pid,
                                                 HYD_pmcd_pmip.local.proxy_core_count,
                                                 HYD_pmcd_pmip.system_global.
                                                 global_core_count);
            else
                pmi_id = HYD_pmcd_pmip.system_global.pmi_id;

            str = HYDU_int_to_str(pmi_id);
            status = HYDU_env_create(&env, "PMI_ID", str);
            HYDU_ERR_POP(status, "unable to create env\n");
            HYDU_FREE(str);
            status = HYDU_append_env_to_list(*env, &force_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");

            for (j = 0, arg = 0; exec->exec[j]; j++)
                client_args[arg++] = HYDU_strdup(exec->exec[j]);
            client_args[arg++] = NULL;

            os_index = HYDT_bind_get_os_index(process_id);
            if (pmi_id == 0) {
                status = HYDU_create_process(client_args, opt_env, force_env,
                                             HYD_pmcd_pmip.system_global.enable_stdin ?
                                             &HYD_pmcd_pmip.downstream.in : NULL,
                                             &HYD_pmcd_pmip.downstream.out[process_id],
                                             &HYD_pmcd_pmip.downstream.err[process_id],
                                             &HYD_pmcd_pmip.downstream.pid[process_id],
                                             os_index);
                HYDU_ERR_POP(status, "create process returned error\n");

                if (HYD_pmcd_pmip.system_global.enable_stdin) {
                    stdin_fd = STDIN_FILENO;
                    status = HYDT_dmx_register_fd(1, &stdin_fd, HYD_POLLIN, NULL,
                                                  HYD_pmcd_pmip_stdin_cb);
                    HYDU_ERR_POP(status, "unable to register fd\n");
                }
            }
            else {
                status = HYDU_create_process(client_args, opt_env, force_env, NULL,
                                             &HYD_pmcd_pmip.downstream.out[process_id],
                                             &HYD_pmcd_pmip.downstream.err[process_id],
                                             &HYD_pmcd_pmip.downstream.pid[process_id],
                                             os_index);
                HYDU_ERR_POP(status, "create process returned error\n");
            }

            process_id++;
        }

        HYDU_env_free_list(force_env);
        force_env = NULL;
    }

    /* Send the PID list upstream */
    cmd = PID_LIST;
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &cmd, sizeof(cmd));
    HYDU_ERR_POP(status, "unable to send PID_LIST command upstream\n");

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control,
                             HYD_pmcd_pmip.downstream.pid,
                             HYD_pmcd_pmip.local.proxy_process_count * sizeof(int));
    HYDU_ERR_POP(status, "unable to send PID list upstream\n");

  fn_spawn_complete:
    /* Everything is spawned, register the required FDs  */
    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.out,
                                  HYD_POLLIN, NULL, HYD_pmcd_pmip_stdout_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.err,
                                  HYD_POLLIN, NULL, HYD_pmcd_pmip_stderr_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    if (pmi_ids)
        HYDU_FREE(pmi_ids);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status parse_exec_params(char **t_argv)
{
    char **argv = t_argv;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    do {
        /* Get the executable arguments  */
        status = HYDU_parse_array(&argv, HYD_pmcd_pmip_match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        /* No more arguments left */
        if (!(*argv))
            break;
    } while (1);

    /* verify the arguments we got */
    if (HYD_pmcd_pmip.system_global.global_core_count == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "global core count not available\n");

    if (HYD_pmcd_pmip.local.proxy_core_count == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "proxy core count not available\n");

    if (HYD_pmcd_pmip.start_pid == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "start PID not available\n");

    if (HYD_pmcd_pmip.exec_list == NULL && HYD_pmcd_pmip.user_global.ckpoint_prefix == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "no executable given and doesn't look like a restart either\n");

    /* Set default values */
    if (HYD_pmcd_pmip.user_global.binding && HYD_pmcd_pmip.local.local_binding == NULL)
        HYD_pmcd_pmip.user_global.binding = HYDU_strdup("none");

    if (HYD_pmcd_pmip.user_global.bindlib == NULL)
        HYD_pmcd_pmip.user_global.bindlib = HYDU_strdup(HYDRA_DEFAULT_BINDLIB);

    if (HYD_pmcd_pmip.user_global.ckpointlib == NULL)
        HYD_pmcd_pmip.user_global.ckpointlib = HYDU_strdup(HYDRA_DEFAULT_CKPOINTLIB);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status procinfo(int fd)
{
    char **arglist;
    int num_strings, str_len, recvd, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Read information about the application to launch into a string
     * array and call parse_exec_params() to interpret it and load it into
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
    status = parse_exec_params(arglist);
    HYDU_ERR_POP(status, "unable to parse argument list\n");

    HYDU_free_strlist(arglist);
    HYDU_FREE(arglist);

    /* Save this fd as we need to send back the exit status on
     * this. */
    HYD_pmcd_pmip.upstream.control = fd;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmip_control_cmd_cb(int fd, HYD_event_t events, void *userp)
{
    int cmd_len;
    enum HYD_pmcd_pmi_cmd cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We got a command from upstream */
    status = HYDU_sock_read(fd, &cmd, sizeof(cmd), &cmd_len, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error reading command from launcher\n");

    if (cmd_len == 0) {
        /* The connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(fd);
        goto fn_exit;
    }

    if (cmd == PROC_INFO) {
        status = procinfo(fd);
        HYDU_ERR_POP(status, "error parsing process info\n");

        status = launch_procs();
        HYDU_ERR_POP(status, "launch_procs returned error\n");
    }
    else if (cmd == KILL_JOB) {
        HYD_pmcd_pmip_killjob();
        status = HYD_SUCCESS;
    }
    else if (cmd == CKPOINT) {
        HYDU_dump(stdout, "requesting checkpoint\n");
        status = HYDT_ckpoint_suspend();
        HYDU_dump(stdout, "checkpoint completed\n");
    }
    else if (cmd == PMI_RESPONSE) {
        status = handle_pmi_response(fd);
        HYDU_ERR_POP(status, "unable to handle PMI response\n");
    }
    else {
        status = HYD_INTERNAL_ERROR;
    }

    HYDU_ERR_POP(status, "error handling proxy command\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmip_stdout_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, STDOUT_FILENO, &closed);
    HYDU_ERR_POP(status, "stdout forwarding error\n");

    if (closed) {
        /* The connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
            if (HYD_pmcd_pmip.downstream.out[i] == fd)
                HYD_pmcd_pmip.downstream.out[i] = -1;

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmip_stderr_cb(int fd, HYD_event_t events, void *userp)
{
    int closed, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, STDERR_FILENO, &closed);
    HYDU_ERR_POP(status, "stderr forwarding error\n");

    if (closed) {
        /* The connection has closed */
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
            if (HYD_pmcd_pmip.downstream.err[i] == fd)
                HYD_pmcd_pmip.downstream.err[i] = -1;

        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmip_stdin_cb(int fd, HYD_event_t events, void *userp)
{
    int closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_forward_stdio(fd, HYD_pmcd_pmip.downstream.in, &closed);
    HYDU_ERR_POP(status, "stdin forwarding error\n");

    if (closed) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");

        close(fd);

        close(HYD_pmcd_pmip.downstream.in);
        HYD_pmcd_pmip.downstream.in = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
