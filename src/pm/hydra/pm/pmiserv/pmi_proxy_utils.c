/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmi_proxy.h"
#include "bsci.h"
#include "bind.h"
#include "ckpoint.h"
#include "hydra_utils.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;
static const char *iface_ip = NULL;

static HYD_status init_params(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_init_user_global(&HYD_pmcd_pmip.user_global);

    HYD_pmcd_pmip.system_global.enable_stdin = -1;
    HYD_pmcd_pmip.system_global.global_core_count = -1;
    HYD_pmcd_pmip.system_global.pmi_port = NULL;
    HYD_pmcd_pmip.system_global.pmi_id = -1;

    HYD_pmcd_pmip.upstream.server_name = NULL;
    HYD_pmcd_pmip.upstream.server_port = -1;
    HYD_pmcd_pmip.upstream.control = -1;

    HYD_pmcd_pmip.downstream.out = NULL;
    HYD_pmcd_pmip.downstream.err = NULL;
    HYD_pmcd_pmip.downstream.in = -1;
    HYD_pmcd_pmip.downstream.pid = NULL;
    HYD_pmcd_pmip.downstream.exit_status = NULL;

    HYD_pmcd_pmip.local.id = -1;
    HYD_pmcd_pmip.local.pgid = -1;
    HYD_pmcd_pmip.local.interface_env_name = NULL;
    HYD_pmcd_pmip.local.hostname = NULL;
    HYD_pmcd_pmip.local.local_binding = NULL;
    HYD_pmcd_pmip.local.proxy_core_count = -1;
    HYD_pmcd_pmip.local.proxy_process_count = -1;

    HYD_pmcd_pmip.start_pid = -1;
    HYD_pmcd_pmip.exec_list = NULL;

    return status;
}

HYD_status HYD_pmcd_pmi_proxy_cleanup_params(void)
{
    struct HYD_exec *exec, *texec;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_pmcd_pmip.upstream.server_name)
        HYDU_FREE(HYD_pmcd_pmip.upstream.server_name);

    if (HYD_pmcd_pmip.user_global.bootstrap)
        HYDU_FREE(HYD_pmcd_pmip.user_global.bootstrap);

    if (HYD_pmcd_pmip.user_global.bootstrap_exec)
        HYDU_FREE(HYD_pmcd_pmip.user_global.bootstrap_exec);

    if (HYD_pmcd_pmip.system_global.pmi_port)
        HYDU_FREE(HYD_pmcd_pmip.system_global.pmi_port);

    if (HYD_pmcd_pmip.user_global.binding)
        HYDU_FREE(HYD_pmcd_pmip.user_global.binding);

    if (HYD_pmcd_pmip.user_global.bindlib)
        HYDU_FREE(HYD_pmcd_pmip.user_global.bindlib);

    if (HYD_pmcd_pmip.user_global.ckpointlib)
        HYDU_FREE(HYD_pmcd_pmip.user_global.ckpointlib);

    if (HYD_pmcd_pmip.user_global.ckpoint_prefix)
        HYDU_FREE(HYD_pmcd_pmip.user_global.ckpoint_prefix);

    if (HYD_pmcd_pmip.user_global.demux)
        HYDU_FREE(HYD_pmcd_pmip.user_global.demux);

    if (HYD_pmcd_pmip.user_global.iface)
        HYDU_FREE(HYD_pmcd_pmip.user_global.iface);

    if (HYD_pmcd_pmip.user_global.global_env.system)
        HYDU_env_free_list(HYD_pmcd_pmip.user_global.global_env.system);

    if (HYD_pmcd_pmip.user_global.global_env.user)
        HYDU_env_free_list(HYD_pmcd_pmip.user_global.global_env.user);

    if (HYD_pmcd_pmip.user_global.global_env.inherited)
        HYDU_env_free_list(HYD_pmcd_pmip.user_global.global_env.inherited);

    if (HYD_pmcd_pmip.exec_list) {
        exec = HYD_pmcd_pmip.exec_list;
        while (exec) {
            texec = exec->next;
            HYDU_free_strlist(exec->exec);
            if (exec->user_env)
                HYDU_env_free(exec->user_env);
            HYDU_FREE(exec);
            exec = texec;
        }
    }

    if (HYD_pmcd_pmip.downstream.pid)
        HYDU_FREE(HYD_pmcd_pmip.downstream.pid);

    if (HYD_pmcd_pmip.downstream.out)
        HYDU_FREE(HYD_pmcd_pmip.downstream.out);

    if (HYD_pmcd_pmip.downstream.err)
        HYDU_FREE(HYD_pmcd_pmip.downstream.err);

    if (HYD_pmcd_pmip.downstream.exit_status)
        HYDU_FREE(HYD_pmcd_pmip.downstream.exit_status);

    if (HYD_pmcd_pmip.local.interface_env_name)
        HYDU_FREE(HYD_pmcd_pmip.local.interface_env_name);

    if (HYD_pmcd_pmip.local.hostname)
        HYDU_FREE(HYD_pmcd_pmip.local.hostname);

    if (HYD_pmcd_pmip.local.local_binding)
        HYDU_FREE(HYD_pmcd_pmip.local.local_binding);

    HYDT_bind_finalize();

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status control_port_fn(char *arg, char ***argv)
{
    char *port = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_pmcd_pmip.upstream.server_name, HYD_INTERNAL_ERROR,
                        "duplicate control port setting\n");

    port = HYDU_strdup(**argv);
    HYD_pmcd_pmip.upstream.server_name = HYDU_strdup(strtok(port, ":"));
    HYD_pmcd_pmip.upstream.server_port = atoi(strtok(NULL, ":"));

    (*argv)++;

  fn_exit:
    if (port)
        HYDU_FREE(port);
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status proxy_id_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.local.id);
}

static HYD_status pgid_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.local.pgid);
}

static HYD_status debug_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_pmcd_pmip.user_global.debug, 1);
}

static HYD_status bootstrap_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.bootstrap);
}

static HYD_status demux_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.demux);
}

static HYD_status iface_fn(char *arg, char ***argv)
{
    struct ifaddrs *ifaddr, *ifa;
    char buf[MAX_HOSTNAME_LEN];
    struct sockaddr_in *sa;
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.iface);
    HYDU_ERR_POP(status, "unable to get the network interface name\n");

    /* Got the interface name; let's query for the IP address */
    if (getifaddrs(&ifaddr) == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "getifaddrs failed\n");

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next)
        if (!strcmp(ifa->ifa_name, HYD_pmcd_pmip.user_global.iface) &&
            (ifa->ifa_addr->sa_family == AF_INET))
            break;

    if (!ifa)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "unable to find interface %s\n",
                             HYD_pmcd_pmip.user_global.iface);

    sa = (struct sockaddr_in *) ifa->ifa_addr;
    iface_ip = inet_ntop(AF_INET, (void *) &(sa->sin_addr), buf, MAX_HOSTNAME_LEN);
    if (!iface_ip)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "unable to find IP for interface %s\n",
                             HYD_pmcd_pmip.user_global.iface);

    freeifaddrs(ifaddr);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status enable_stdin_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.system_global.enable_stdin);
}

static HYD_status pmi_port_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.system_global.pmi_port);
}

static HYD_status pmi_id_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.system_global.pmi_id);
}

static HYD_status binding_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.binding);
}

static HYD_status bindlib_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.bindlib);
}

static HYD_status ckpointlib_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.ckpointlib);
}

static HYD_status ckpoint_prefix_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.ckpoint_prefix);
}

static HYD_status global_env_fn(char *arg, char ***argv)
{
    int i, count;
    char *str;
    struct HYD_env *env;
    HYD_status status = HYD_SUCCESS;

    count = atoi(**argv);
    for (i = 0; i < count; i++) {
        (*argv)++;
        str = **argv;

        /* Environment variables are quoted; remove them */
        if (*str == '\'') {
            str++;
            str[strlen(str) - 1] = 0;
        }
        status = HYDU_str_to_env(str, &env);
        HYDU_ERR_POP(status, "error converting string to env\n");

        if (!strcmp(arg, "global-inherited-env"))
            HYDU_append_env_to_list(*env, &HYD_pmcd_pmip.user_global.global_env.inherited);
        else if (!strcmp(arg, "global-system-env"))
            HYDU_append_env_to_list(*env, &HYD_pmcd_pmip.user_global.global_env.system);
        else if (!strcmp(arg, "global-user-env"))
            HYDU_append_env_to_list(*env, &HYD_pmcd_pmip.user_global.global_env.user);

        HYDU_FREE(env);
    }
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status genv_prop_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.user_global.global_env.prop);
}

static HYD_status global_core_count_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.system_global.global_core_count);
}

static HYD_status version_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (strcmp(**argv, HYDRA_VERSION)) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "UI version string does not match proxy version\n");
    }
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status interface_env_name_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.local.interface_env_name);
}

static HYD_status hostname_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.local.hostname);
}

static HYD_status local_binding_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_pmcd_pmip.local.local_binding);
}

static HYD_status proxy_core_count_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.local.proxy_core_count);
}

static HYD_status start_pid_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_pmcd_pmip.start_pid);
}

static HYD_status exec_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    if (HYD_pmcd_pmip.exec_list == NULL) {
        status = HYDU_alloc_exec(&HYD_pmcd_pmip.exec_list);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");
    }
    else {
        for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);
        status = HYDU_alloc_exec(&exec->next);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status exec_proc_count_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);
    return HYDU_set_int_and_incr(arg, argv, &exec->proc_count);
}

static HYD_status exec_local_env_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    struct HYD_env *env;
    int i, count;
    char *str;
    HYD_status status = HYD_SUCCESS;

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "NULL argument to exec local env\n");

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    count = atoi(**argv);
    for (i = 0; i < count; i++) {
        (*argv)++;
        str = **argv;

        /* Environment variables are quoted; remove them */
        if (*str == '\'') {
            str++;
            str[strlen(str) - 1] = 0;
        }
        status = HYDU_str_to_env(str, &env);
        HYDU_ERR_POP(status, "error converting string to env\n");
        HYDU_append_env_to_list(*env, &exec->user_env);
        HYDU_FREE(env);
    }
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status exec_env_prop_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    return HYDU_set_str_and_incr(arg, argv, &exec->env_prop);
}

static HYD_status exec_wdir_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    return HYDU_set_str_and_incr(arg, argv, &exec->wdir);
}

static HYD_status exec_args_fn(char *arg, char ***argv)
{
    int i, count;
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "NULL argument to exec args\n");

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    count = atoi(**argv);
    for (i = 0; i < count; i++) {
        (*argv)++;
        exec->exec[i] = HYDU_strdup(**argv);
    }
    exec->exec[i] = NULL;
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_arg_match_table match_table[] = {
    /* Proxy parameters */
    {"control-port", control_port_fn, NULL},
    {"proxy-id", proxy_id_fn, NULL},
    {"pgid", pgid_fn, NULL},
    {"debug", debug_fn, NULL},
    {"bootstrap", bootstrap_fn, NULL},
    {"demux", demux_fn, NULL},
    {"iface", iface_fn, NULL},
    {"enable-stdin", enable_stdin_fn, NULL},

    /* Executable parameters */
    {"pmi-port", pmi_port_fn, NULL},
    {"pmi-id", pmi_id_fn, NULL},
    {"binding", binding_fn, NULL},
    {"bindlib", bindlib_fn, NULL},
    {"ckpointlib", ckpointlib_fn, NULL},
    {"ckpoint-prefix", ckpoint_prefix_fn, NULL},
    {"global-inherited-env", global_env_fn, NULL},
    {"global-system-env", global_env_fn, NULL},
    {"global-user-env", global_env_fn, NULL},
    {"genv-prop", genv_prop_fn, NULL},
    {"global-core-count", global_core_count_fn, NULL},
    {"version", version_fn, NULL},
    {"interface-env-name", interface_env_name_fn, NULL},
    {"hostname", hostname_fn, NULL},
    {"local-binding", local_binding_fn, NULL},
    {"proxy-core-count", proxy_core_count_fn, NULL},
    {"start-pid", start_pid_fn, NULL},
    {"exec", exec_fn, NULL},
    {"exec-proc-count", exec_proc_count_fn, NULL},
    {"exec-local-env", exec_local_env_fn, NULL},
    {"exec-env-prop", exec_env_prop_fn, NULL},
    {"exec-wdir", exec_wdir_fn, NULL},
    {"exec-args", exec_args_fn, NULL}
};

HYD_status HYD_pmcd_pmi_proxy_get_params(char **t_argv)
{
    char **argv = t_argv;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = init_params();
    HYDU_ERR_POP(status, "Error initializing proxy params\n");

    argv++;
    do {
        /* Get the proxy arguments  */
        status = HYDU_parse_array(&argv, match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        /* No more arguments left */
        if (!(*argv))
            break;
    } while (1);

    /* Verify the arguments we got */
    if (HYD_pmcd_pmip.upstream.server_name == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "server name not available\n");

    if (HYD_pmcd_pmip.upstream.server_port == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "server port not available\n");

    if (HYD_pmcd_pmip.user_global.demux == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "demux engine not available\n");

    status = HYDT_dmx_init(&HYD_pmcd_pmip.user_global.demux);
    HYDU_ERR_POP(status, "unable to initialize the demux engine\n");

    if (HYD_pmcd_pmip.user_global.bootstrap == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "bootstrap server not available\n");

    status = HYDT_bsci_init(HYD_pmcd_pmip.user_global.bootstrap, NULL /* no bootstrap exec */ ,
                            0 /* disable x */ , HYD_pmcd_pmip.user_global.debug);
    HYDU_ERR_POP(status, "proxy unable to initialize bootstrap\n");

    if (HYD_pmcd_pmip.local.id == -1) {
        /* We didn't get a proxy ID during launch; query the bootstrap
         * server for it. */
        status = HYDT_bsci_query_proxy_id(&HYD_pmcd_pmip.local.id);
        HYDU_ERR_POP(status, "unable to query bootstrap server for proxy ID\n");
    }

    if (HYD_pmcd_pmip.local.id == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "proxy ID not available\n");

    if (HYD_pmcd_pmip.local.pgid == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PG ID not available\n");

  fn_exit:
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
        status = HYDU_parse_array(&argv, match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        /* No more arguments left */
        if (!(*argv))
            break;
    } while (1);

    /* verify the arguments we got */
    if (HYD_pmcd_pmip.system_global.pmi_port == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI port not available\n");

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

HYD_status HYD_pmcd_pmi_proxy_procinfo(int fd)
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

HYD_status HYD_pmcd_pmi_proxy_launch_procs(void)
{
    int i, j, arg, stdin_fd, process_id, os_index, pmi_id;
    char *str, *envstr, *list;
    char *client_args[HYD_NUM_TMP_STRINGS];
    struct HYD_env *env, *prop_env = NULL;
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;
    int *pmi_ids;

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

    status = HYDT_bind_init(HYD_pmcd_pmip.local.local_binding ? HYD_pmcd_pmip.local.local_binding :
                            HYD_pmcd_pmip.user_global.binding, HYD_pmcd_pmip.user_global.bindlib);
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

    /* Spawn the processes */
    process_id = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {

        /* Increasing priority order: (1) global inherited env; (2)
         * global user env; (3) local user env; (4) system env. We
         * just set them one after the other, overwriting the previous
         * written value if needed. */

        /* global inherited env */
        if ((exec->env_prop && !strcmp(exec->env_prop, "all")) ||
            (!exec->env_prop && !strcmp(HYD_pmcd_pmip.user_global.global_env.prop, "all"))) {
            for (env = HYD_pmcd_pmip.user_global.global_env.inherited; env; env = env->next) {
                status = HYDU_append_env_to_list(*env, &prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
        }
        else if ((exec->env_prop && !strncmp(exec->env_prop, "list", strlen("list"))) ||
                 (!exec->env_prop &&
                  !strncmp(HYD_pmcd_pmip.user_global.global_env.prop, "list",
                           strlen("list")))) {
            if (exec->env_prop)
                list = HYDU_strdup(exec->env_prop + strlen("list:"));
            else
                list = HYDU_strdup(HYD_pmcd_pmip.user_global.global_env.prop +
                                   strlen("list:"));

            envstr = strtok(list, ",");
            while (envstr) {
                env = HYDU_env_lookup(envstr, HYD_pmcd_pmip.user_global.global_env.inherited);
                if (env) {
                    status = HYDU_append_env_to_list(*env, &prop_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                }
                envstr = strtok(NULL, ",");
            }
        }

        /* global user env */
        for (env = HYD_pmcd_pmip.user_global.global_env.user; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &prop_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* local user env */
        for (env = exec->user_env; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &prop_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* system env */
        for (env = HYD_pmcd_pmip.user_global.global_env.system; env; env = env->next) {
            status = HYDU_append_env_to_list(*env, &prop_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");
        }

        /* Set the PMI port string to connect to. We currently just
         * use the global PMI port. */
        status = HYDU_env_create(&env, "PMI_PORT", HYD_pmcd_pmip.system_global.pmi_port);
        HYDU_ERR_POP(status, "unable to create env\n");

        status = HYDU_append_env_to_list(*env, &prop_env);
        HYDU_ERR_POP(status, "unable to add env to list\n");

        /* Set the interface hostname based on what the user provided */
        if (HYD_pmcd_pmip.local.interface_env_name) {
            if (iface_ip) {
                /* The user asked us to use a specific interface; let's find it */
                status = HYDU_env_create(&env, HYD_pmcd_pmip.local.interface_env_name,
                                         (char *) iface_ip);
                HYDU_ERR_POP(status, "unable to create env\n");

                status = HYDU_append_env_to_list(*env, &prop_env);
                HYDU_ERR_POP(status, "unable to add env to list\n");
            }
            else if (HYD_pmcd_pmip.local.hostname) {
                /* The second choice is the hostname the user gave */
                status = HYDU_env_create(&env, HYD_pmcd_pmip.local.interface_env_name,
                                         HYD_pmcd_pmip.local.hostname);
                HYDU_ERR_POP(status, "unable to create env\n");

                status = HYDU_append_env_to_list(*env, &prop_env);
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
                                                 HYD_pmcd_pmip.
                                                 system_global.global_core_count);
            else
                pmi_id = HYD_pmcd_pmip.system_global.pmi_id;

            str = HYDU_int_to_str(pmi_id);
            status = HYDU_env_create(&env, "PMI_ID", str);
            HYDU_ERR_POP(status, "unable to create env\n");
            HYDU_FREE(str);
            status = HYDU_append_env_to_list(*env, &prop_env);
            HYDU_ERR_POP(status, "unable to add env to list\n");

            for (j = 0, arg = 0; exec->exec[j]; j++)
                client_args[arg++] = HYDU_strdup(exec->exec[j]);
            client_args[arg++] = NULL;

            os_index = HYDT_bind_get_os_index(process_id);
            if (pmi_id == 0) {
                status = HYDU_create_process(client_args, prop_env,
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
                                                  HYD_pmcd_pmi_proxy_stdin_cb);
                    HYDU_ERR_POP(status, "unable to register fd\n");
                }
            }
            else {
                status = HYDU_create_process(client_args, prop_env, NULL,
                                             &HYD_pmcd_pmip.downstream.out[process_id],
                                             &HYD_pmcd_pmip.downstream.err[process_id],
                                             &HYD_pmcd_pmip.downstream.pid[process_id],
                                             os_index);
                HYDU_ERR_POP(status, "create process returned error\n");
            }

            process_id++;
        }

        HYDU_env_free_list(prop_env);
        prop_env = NULL;
    }

  fn_spawn_complete:
    /* Everything is spawned, register the required FDs  */
    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.out,
                                  HYD_POLLIN, NULL, HYD_pmcd_pmi_proxy_stdout_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

    status = HYDT_dmx_register_fd(HYD_pmcd_pmip.local.proxy_process_count,
                                  HYD_pmcd_pmip.downstream.err,
                                  HYD_POLLIN, NULL, HYD_pmcd_pmi_proxy_stderr_cb);
    HYDU_ERR_POP(status, "unable to register fd\n");

  fn_exit:
    if (pmi_ids)
        HYDU_FREE(pmi_ids);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_pmi_proxy_killjob(void)
{
    int i;

    HYDU_FUNC_ENTER();

    /* Send the kill signal to all processes */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pid[i] != -1) {
            kill(HYD_pmcd_pmip.downstream.pid[i], SIGTERM);
            kill(HYD_pmcd_pmip.downstream.pid[i], SIGKILL);
        }
    }

    HYDU_FUNC_EXIT();
}
