/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "uiu.h"

#define IS_HELP(str) \
    ((!strcmp((str), "-h")) || (!strcmp((str), "-help")) || (!strcmp((str), "--help")))

struct HYD_handle HYD_handle;

static void dump_env_notes(void)
{
    printf("Additional generic notes:\n");
    printf("  * There are three types of environments: system, user and inherited\n");
    printf("      --> system env is automatically set by Hydra and internally used\n");
    printf("      --> user env is set by -genv, -env, -genvlist and -envlist\n");
    printf("      --> inherited env is taken from the user shell; it can be controlled\n");
    printf("               using the options -genvall, -envall, -genvnone, -envnone\n");
    printf("               NOTE: these options do not affect the user set environment\n");
    printf("  * Multiple global env options cannot be used together\n");
    printf("  * Multiple local env options cannot be used for the same executable\n");
    printf("  * Local inherited env options override the global options\n");
    printf("\n");
}

static HYD_status genv_fn(char *arg, char ***argv)
{
    char *env_name, *env_value, *str[2] = { 0 };
    HYD_env_t *env;
    HYD_status status = HYD_SUCCESS;

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-genv: Specify an ENV-VALUE pair to be passed to each process\n\n");
        printf("Notes:\n");
        printf("  * Usage: -genv env1 val1\n");
        printf("  * Can be used multiple times for different variables\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    status = HYDU_strsplit(**argv, &str[0], &str[1], '=');
    HYDU_ERR_POP(status, "string break returned error\n");
    (*argv)++;

    env_name = HYDU_strdup(str[0]);
    if (str[1] == NULL) {       /* The environment is not of the form x=foo */
        if (**argv == NULL) {
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
        env_value = HYDU_strdup(**argv);
        (*argv)++;
    }
    else {
        env_value = HYDU_strdup(str[1]);
    }

    status = HYDU_env_create(&env, env_name, env_value);
    HYDU_ERR_POP(status, "unable to create env struct\n");

    HYDU_append_env_to_list(*env, &HYD_handle.user_global.global_env.user);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status genvlist_fn(char *arg, char ***argv)
{
    int len;
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-genvlist: A list of ENV variables is passed to each process\n\n");
        printf("Notes:\n");
        printf("  * Usage: -genvlist env1,env2,env2\n");
        printf("  * Values of env1,env2,env3 are taken from the user ENV\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC(HYD_handle.user_global.global_env.prop, char *, len, status);
    HYDU_snprintf(HYD_handle.user_global.global_env.prop, len, "list:%s", **argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status genvnone_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-genvnone: None of the ENV is passed to each process\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.global_env.prop = HYDU_strdup("none");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status genvall_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-genvall: All of the ENV is passed to each process (default)\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.global_env.prop = HYDU_strdup("all");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status process_mfile_token(char *token, int newline)
{
    int num_procs;
    char *hostname, *procs;
    HYD_status status = HYD_SUCCESS;

    if (newline) {      /* The first entry gives the hostname and processes */
        hostname = strtok(token, ":");
        procs = strtok(NULL, ":");
        num_procs = procs ? atoi(procs) : 1;

        status = HYDU_add_to_node_list(hostname, num_procs, &HYD_handle.node_list);
        HYDU_ERR_POP(status, "unable to initialize proxy\n");
        HYD_handle.global_core_count += num_procs;
    }
    else {      /* Not a new line */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "token %s not supported at this time\n", token);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status mfile_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.pg_list.proxy_list, HYD_INTERNAL_ERROR,
                        "duplicate host file setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-f: Host file containing compute node names\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (strcmp(**argv, "HYDRA_USE_LOCALHOST")) {
        status = HYDU_parse_hostfile(**argv, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }
    else {
        status = HYDU_add_to_node_list((char *) "localhost", 1, &HYD_handle.node_list);
        HYDU_ERR_POP(status, "unable to add proxy\n");
        HYD_handle.global_core_count += 1;
    }

    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status wdir_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.wdir, HYD_INTERNAL_ERROR,
                        "duplicate wdir setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-wdir: Working directory to use on the compute nodes\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.wdir = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status env_fn(char *arg, char ***argv)
{
    char *env_name, *env_value, *str[2] = { 0 };
    HYD_env_t *env;
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-env: Specify an ENV-VALUE pair to be passed to this executable\n\n");
        printf("Notes:\n");
        printf("  * Usage: -env env1 val1\n");
        printf("  * Can be used multiple times for different variables\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    status = HYDU_strsplit(**argv, &str[0], &str[1], '=');
    HYDU_ERR_POP(status, "string break returned error\n");
    (*argv)++;

    env_name = HYDU_strdup(str[0]);
    if (str[1] == NULL) {       /* The environment is not of the form x=foo */
        if (**argv == NULL) {
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
        env_value = HYDU_strdup(**argv);
        (*argv)++;
    }
    else {
        env_value = HYDU_strdup(str[1]);
    }

    status = HYDU_env_create(&env, env_name, env_value);
    HYDU_ERR_POP(status, "unable to create env struct\n");

    status = HYD_uiu_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    HYDU_append_env_to_list(*env, &exec_info->user_env);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status envlist_fn(char *arg, char ***argv)
{
    int len;
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    status = HYD_uiu_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    HYDU_ERR_CHKANDJUMP(status, exec_info->env_prop, HYD_INTERNAL_ERROR,
                        "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-envlist: A list of ENV variables is passed to this executable\n\n");
        printf("Notes:\n");
        printf("  * Usage: -envlist env1,env2,env2\n");
        printf("  * Values of env1,env2,env3 are taken from the user ENV\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC(exec_info->env_prop, char *, len, status);
    HYDU_snprintf(exec_info->env_prop, len, "list:%s", **argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status envnone_fn(char *arg, char ***argv)
{
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    status = HYD_uiu_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    HYDU_ERR_CHKANDJUMP(status, exec_info->env_prop, HYD_INTERNAL_ERROR,
                        "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-envnone: None of the ENV is passed to this executable\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    exec_info->env_prop = HYDU_strdup("none");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status envall_fn(char *arg, char ***argv)
{
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    status = HYD_uiu_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    HYDU_ERR_CHKANDJUMP(status, exec_info->env_prop, HYD_INTERNAL_ERROR,
                        "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-envall: All of the ENV is passed to this executable\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    exec_info->env_prop = HYDU_strdup("all");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status np_fn(char *arg, char ***argv)
{
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-n: Number of processes to launch\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    status = HYD_uiu_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    if (exec_info->process_count != 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate process count\n");

    exec_info->process_count = atoi(**argv);
    HYD_handle.pg_list.pg_process_count += exec_info->process_count;
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status bootstrap_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.bootstrap, HYD_INTERNAL_ERROR,
                        "duplicate -bootstrap option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no bootstrap server specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-bootstrap: Bootstrap server to use\n\n");
        printf("Notes:\n");
        printf("  * Use the -info option to see what all are compiled in\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.bootstrap = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status bootstrap_exec_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.bootstrap_exec, HYD_INTERNAL_ERROR,
                        "duplicate -bootstrap-exec option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no bootstrap exec specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-bootstrap-exec: Bootstrap executable to use\n\n");
        printf("Notes:\n");
        printf("  * This is needed only if Hydra cannot automatically find it\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.bootstrap_exec = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status enablex_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.enablex != -1, HYD_INTERNAL_ERROR,
                        "duplicate -enable-x argument\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-enable-x: Enable X forwarding\n");
        printf("-disable-x: Disable X forwarding\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (!strcmp(arg, "enable-x"))
        HYD_handle.user_global.enablex = 1;
    else
        HYD_handle.user_global.enablex = 0;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status boot_proxies_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.launch_mode != HYD_LAUNCH_UNSET,
                        HYD_INTERNAL_ERROR, "duplicate launch mode\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-boot-proxies: Launch persistent proxies\n");
        printf("-boot-foreground-proxies: ");
        printf("Launch persistent proxies in foreground\n");
        printf("-shutdown-proxies: Shutdown persistent proxies\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (!strcmp(arg, "boot-proxies"))
        HYD_handle.user_global.launch_mode = HYD_LAUNCH_BOOT;
    else if (!strcmp(arg, "boot-foreground-proxies"))
        HYD_handle.user_global.launch_mode = HYD_LAUNCH_BOOT_FOREGROUND;
    else if (!strcmp(arg, "shutdown-proxies"))
        HYD_handle.user_global.launch_mode = HYD_LAUNCH_SHUTDOWN;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status proxy_port_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.proxy_port != -1, HYD_INTERNAL_ERROR,
                        "duplicate -proxy-port option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no proxy port specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-proxy-port: Port for persistent proxies to listen on\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.proxy_port = atoi(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status use_persistent_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.launch_mode != HYD_LAUNCH_UNSET,
                        HYD_INTERNAL_ERROR, "duplicate launch mode\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-use-persistent: Use persistent proxies\n");
        printf("-use-runtime: Launch proxies at runtime\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (!strcmp(arg, "use-persistent"))
        HYD_handle.user_global.launch_mode = HYD_LAUNCH_PERSISTENT;
    else
        HYD_handle.user_global.launch_mode = HYD_LAUNCH_RUNTIME;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status css_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.css, HYD_INTERNAL_ERROR, "duplicate -css option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no css specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-css: Communication sub-system to use\n\n");
        printf("Notes:\n");
        printf("  * Use the -info option to see what all are compiled in\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.css = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status rmk_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.rmk, HYD_INTERNAL_ERROR, "duplicate -rmk option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no rmk specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-rmk: Resource management kernel to use\n\n");
        printf("Notes:\n");
        printf("  * Use the -info option to see what all are compiled in\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.rmk = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ranks_per_proc_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.ranks_per_proc != -1, HYD_INTERNAL_ERROR,
                        "duplicate -ranks-per-proc option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no ranks-per-proc specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ranks-per-proc: MPI ranks to assign per launched process\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.ranks_per_proc = atoi(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status enable_pm_env_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.pm_env != -1, HYD_INTERNAL_ERROR,
                        "duplicate -enable-pm-env argument\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-enable-pm-env: Enable ENV added by the process manager\n");
        printf("-disable-pm-env: Disable ENV added by the process manager\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (!strcmp(arg, "enable-pm-env"))
        HYD_handle.pm_env = 1;
    else
        HYD_handle.pm_env = 0;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status binding_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.binding, HYD_INTERNAL_ERROR,
                        "duplicate binding\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no binding specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-binding: Process-core binding to use\n\n");
        printf("Notes:\n");
        printf("  * Usage: -binding [type]; where [type] can be:\n");
        printf("        none -- no binding\n");
        printf("        rr   -- round-robin as OS assigned processor IDs\n");
        printf("        user:0,1,3,2 -- user specified binding\n");
        printf("        topo -- CPU topology-aware binding\n");
        printf("        topomem -- memory topology-aware binding\n\n");

        printf("    CPU options (supported on topology-capable binding libs):\n");
        printf("        topo (with no options) -- use all CPU resources\n");
        printf("        topo:sockets,cores,threads -- use all CPU resources\n");
        printf("        topo:sockets,cores -- avoid using multiple threads on a core\n");
        printf("        topo:sockets -- avoid using multiple cores on a socket\n");

        printf("    Memory options (supported on memory topology aware binding libs):\n");
        printf("        topomem:l1,l2,l3,mem -- use all memory resources\n");
        printf("        topomem:l2,l3,mem -- avoid sharing l1 cache\n");
        printf("        topomem:l3,mem -- avoid using l2 cache\n");
        printf("        topomem:mem -- avoid using l3 cache\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.binding = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status bindlib_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.bindlib, HYD_INTERNAL_ERROR,
                        "duplicate -bindlib option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no bindlib specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-bindlib: Binding library to use\n\n");
        printf("Notes:\n");
        printf("  * Use the -info option to see what all are compiled in\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.bindlib = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ckpoint_interval_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.ckpoint_int != -1,
                        HYD_INTERNAL_ERROR, "duplicate -ckpoint-interval option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no ckpoint interval specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ckpoint-interval: Checkpointing interval\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.ckpoint_int = atoi(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ckpoint_prefix_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.ckpoint_prefix, HYD_INTERNAL_ERROR,
                        "duplicate -ckpoint-prefix option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no ckpoint prefix specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ckpoint-prefix: Checkpoint file prefix to use\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.ckpoint_prefix = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ckpointlib_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.ckpointlib, HYD_INTERNAL_ERROR,
                        "duplicate -ckpointlib option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no ckpointlib specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ckpointlib: Checkpointing library to use\n\n");
        printf("Notes:\n");
        printf("  * Use the -info option to see what all are compiled in\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.ckpointlib = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ckpoint_restart_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.ckpoint_restart,
                        HYD_INTERNAL_ERROR, "duplicate restart mode\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ckpoint-restart: Restart checkpointed application\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.ckpoint_restart = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status verbose_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.debug != -1, HYD_INTERNAL_ERROR,
                        "duplicate verbose setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-verbose: Prints additional debug information\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.user_global.debug = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status info_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-info: Shows build information\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYDU_dump_noprefix(stdout, "HYDRA build details:\n");
    HYDU_dump_noprefix(stdout,
                       "    Version:                                 %s\n", HYDRA_VERSION);
    HYDU_dump_noprefix(stdout, "    CC:                                      %s\n", HYDRA_CC);
    HYDU_dump_noprefix(stdout, "    CXX:                                     %s\n", HYDRA_CXX);
    HYDU_dump_noprefix(stdout, "    F77:                                     %s\n", HYDRA_F77);
    HYDU_dump_noprefix(stdout, "    F90:                                     %s\n", HYDRA_F90);
    HYDU_dump_noprefix(stdout,
                       "    Configure options:                       %s\n",
                       HYDRA_CONFIGURE_ARGS_CLEAN);
    HYDU_dump_noprefix(stdout, "    Process Manager:                         pmi\n");
    HYDU_dump_noprefix(stdout,
                       "    Boot-strap servers available:            %s\n", HYDRA_BSS_NAMES);
    HYDU_dump_noprefix(stdout, "    Communication sub-systems available:     none\n");
    HYDU_dump_noprefix(stdout,
                       "    Binding libraries available:             %s\n",
                       HYDRA_BINDLIB_NAMES);
    HYDU_dump_noprefix(stdout,
                       "    Checkpointing libraries available:       %s\n",
                       HYDRA_CKPOINTLIB_NAMES);

    HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status print_rank_map_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.print_rank_map != -1, HYD_INTERNAL_ERROR,
                        "duplicate print-rank-map setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-print-rank-map: Print what ranks are allocated to what nodes\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.print_rank_map = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status print_all_exitcodes_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.print_all_exitcodes != -1, HYD_INTERNAL_ERROR,
                        "duplicate print-all-exitcodes setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-print-all-exitcodes: Print termination codes of all processes\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.print_all_exitcodes = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

struct match_table_fns {
    const char *arg;
     HYD_status(*handler) (char *arg, char ***argv_p);
};

static struct match_table_fns match_table[] = {
    /* Global environment options */
    {"genv", genv_fn},
    {"genvlist", genvlist_fn},
    {"genvnone", genvnone_fn},
    {"genvall", genvall_fn},

    /* Other global options */
    {"f", mfile_fn},
    {"machinefile", mfile_fn},
    {"wdir", wdir_fn},

    /* Local environment options */
    {"env", env_fn},
    {"envlist", envlist_fn},
    {"envnone", envnone_fn},
    {"envall", envall_fn},

    /* Other local options */
    {"n", np_fn},
    {"np", np_fn},

    /* Hydra specific options */

    /* Bootstrap options */
    {"bootstrap", bootstrap_fn},
    {"bootstrap-exec", bootstrap_exec_fn},
    {"enable-x", enablex_fn},
    {"disable-x", enablex_fn},

    /* Proxy options */
    {"boot-proxies", boot_proxies_fn},
    {"boot-foreground-proxies", boot_proxies_fn},
    {"shutdown-proxies", boot_proxies_fn},
    {"proxy-port", proxy_port_fn},
    {"use-persistent", use_persistent_fn},
    {"use-runtime", use_persistent_fn},

    /* Communication sub-system options */
    {"css", css_fn},

    /* Resource management kernel options */
    {"rmk", rmk_fn},

    /* Hybrid programming options */
    {"ranks-per-proc", ranks_per_proc_fn},
    {"enable-pm-env", enable_pm_env_fn},
    {"disable-pm-env", enable_pm_env_fn},

    /* Process-core binding options */
    {"binding", binding_fn},
    {"bindlib", bindlib_fn},

    /* Checkpoint/restart options */
    {"ckpoint-interval", ckpoint_interval_fn},
    {"ckpoint-prefix", ckpoint_prefix_fn},
    {"ckpointlib", ckpointlib_fn},
    {"ckpoint-restart", ckpoint_restart_fn},

    /* Other hydra options */
    {"verbose", verbose_fn},
    {"v", verbose_fn},
    {"debug", verbose_fn},
    {"info", info_fn},
    {"print-rank-map", print_rank_map_fn},
    {"print-all-exitcodes", print_all_exitcodes_fn},

    {"\0", NULL}
};

static HYD_status match_arg(char ***argv_p)
{
    struct match_table_fns *m;
    char *arg, *tmp;
    HYD_status status = HYD_SUCCESS;

    arg = **argv_p;
    while (*arg == '-') /* Remove leading dashes */
        arg++;

    /* Find an '=' in the first argument */
    while (((**argv_p)[0] != '\0') && ((**argv_p)[0] != '='))
        (**argv_p)++;
    if ((**argv_p)[0] == '\0')  /* No '=' found */
        (*argv_p)++;
    else        /* '=' found */
        (**argv_p)++;

    /* The argument only runs till the next '=' */
    tmp = arg;
    while (*tmp != '\0' && *tmp != '=')
        tmp++;
    *tmp = '\0';

    m = match_table;
    while (m->handler) {
        if (!strcmp(arg, m->arg)) {
            status = m->handler(arg, argv_p);
            HYDU_ERR_POP(status, "match handler returned error\n");
            break;
        }
        m++;
    }

    if (m->handler == NULL)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "unrecognized argument %s\n", arg);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status verify_arguments(void)
{
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    /* Proxy launch or checkpoint restart */
    if ((HYD_handle.user_global.launch_mode == HYD_LAUNCH_BOOT) ||
        (HYD_handle.user_global.launch_mode == HYD_LAUNCH_BOOT_FOREGROUND) ||
        (HYD_handle.user_global.launch_mode == HYD_LAUNCH_SHUTDOWN)) {

        /* No environment */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.global_env.prop, HYD_INTERNAL_ERROR,
                            "env setting not required for booting/shutting proxies\n");

        /* No binding */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.binding, HYD_INTERNAL_ERROR,
                            "binding not allowed while booting/shutting proxies\n");
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.bindlib, HYD_INTERNAL_ERROR,
                            "binding not allowed while booting/shutting proxies\n");

        /* No executables */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.exec_info_list, HYD_INTERNAL_ERROR,
                            "execs should not be specified while booting/shutting proxies\n");
    }
    else {      /* Application launch */
        /* On a checkpoint restart, we set the prefix as the application */
        if (HYD_handle.user_global.ckpoint_restart == 1) {
            status = HYD_uiu_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            exec_info->exec[0] = HYDU_strdup(HYD_handle.user_global.ckpoint_prefix);
            exec_info->exec[1] = NULL;
        }

        if (HYD_handle.exec_info_list == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        /* Make sure local executable is set */
        for (exec_info = HYD_handle.exec_info_list; exec_info; exec_info = exec_info->next) {
            if (exec_info->exec[0] == NULL)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");
        }
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status set_default_values(void)
{
    char *tmp;
    HYD_status status = HYD_SUCCESS;

    if (HYD_handle.print_rank_map == -1)
        HYD_handle.print_rank_map = 0;

    if (HYD_handle.print_all_exitcodes == -1)
        HYD_handle.print_all_exitcodes = 0;

    tmp = getenv("HYDRA_DEBUG");
    if (HYD_handle.user_global.debug == -1 && tmp)
        HYD_handle.user_global.debug = atoi(tmp) ? 1 : 0;
    if (HYD_handle.user_global.debug == -1)
        HYD_handle.user_global.debug = 0;

    tmp = getenv("HYDRA_PM_ENV");
    if (HYD_handle.pm_env == -1 && tmp)
        HYD_handle.pm_env = (atoi(getenv("HYDRA_PM_ENV")) != 0);
    if (HYD_handle.pm_env == -1)
        HYD_handle.pm_env = 1;  /* Default is to pass the PM environment */

    tmp = getenv("HYDRA_BOOTSTRAP");
    if (HYD_handle.user_global.bootstrap == NULL && tmp)
        HYD_handle.user_global.bootstrap = HYDU_strdup(tmp);
    if (HYD_handle.user_global.bootstrap == NULL)
        HYD_handle.user_global.bootstrap = HYDU_strdup(HYDRA_DEFAULT_BSS);

    tmp = getenv("HYDRA_CSS");
    if (HYD_handle.css == NULL && tmp)
        HYD_handle.css = HYDU_strdup(tmp);
    if (HYD_handle.css == NULL)
        HYD_handle.css = HYDU_strdup(HYDRA_DEFAULT_CSS);

    tmp = getenv("HYDRA_RMK");
    if (HYD_handle.rmk == NULL && tmp)
        HYD_handle.rmk = HYDU_strdup(tmp);
    if (HYD_handle.rmk == NULL)
        HYD_handle.rmk = HYDU_strdup(HYDRA_DEFAULT_RMK);

    tmp = getenv("HYDRA_HOST_FILE");
    if (HYD_handle.pg_list.proxy_list == NULL && tmp) {
        status = HYDU_parse_hostfile(tmp, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }

    tmp = getenv("HYDRA_PROXY_PORT");
    if (HYD_handle.proxy_port == -1 && tmp)
        HYD_handle.proxy_port = atoi(getenv("HYDRA_PROXY_PORT"));
    if (HYD_handle.proxy_port == -1)
        HYD_handle.proxy_port = HYD_DEFAULT_PROXY_PORT;

    tmp = getenv("HYDRA_LAUNCH_MODE");
    if (HYD_handle.user_global.launch_mode == HYD_LAUNCH_UNSET && tmp) {
        if (!strcmp(tmp, "persistent"))
            HYD_handle.user_global.launch_mode = HYD_LAUNCH_PERSISTENT;
        else if (!strcmp(tmp, "runtime"))
            HYD_handle.user_global.launch_mode = HYD_LAUNCH_RUNTIME;
    }
    if (HYD_handle.user_global.launch_mode == HYD_LAUNCH_UNSET)
        HYD_handle.user_global.launch_mode = HYD_LAUNCH_RUNTIME;

    tmp = getenv("HYDRA_BOOT_FOREGROUND_PROXIES");
    if (HYD_handle.user_global.launch_mode == HYD_LAUNCH_UNSET && tmp) {
        if (atoi(tmp) == 1) {
            HYD_handle.user_global.launch_mode = HYD_LAUNCH_BOOT_FOREGROUND;
        }
    }

    tmp = getenv("HYDRA_BOOTSTRAP_EXEC");
    if (HYD_handle.user_global.bootstrap_exec == NULL && tmp)
        HYD_handle.user_global.bootstrap_exec = HYDU_strdup(tmp);

    /* Check environment for setting binding */
    tmp = getenv("HYDRA_BINDING");
    if (HYD_handle.user_global.binding == NULL && tmp)
        HYD_handle.user_global.binding = HYDU_strdup(tmp);

    tmp = getenv("HYDRA_BINDLIB");
    if (HYD_handle.user_global.bindlib == NULL && tmp)
        HYD_handle.user_global.bindlib = HYDU_strdup(tmp);
    if (HYD_handle.user_global.bindlib == NULL && strcmp(HYDRA_DEFAULT_BINDLIB, ""))
        HYD_handle.user_global.bindlib = HYDU_strdup(HYDRA_DEFAULT_BINDLIB);
    if (HYD_handle.user_global.bindlib == NULL)
        HYD_handle.user_global.bindlib = HYDU_strdup("none");

    /* Check environment for checkpointing */
    tmp = getenv("HYDRA_CKPOINTLIB");
    if (HYD_handle.user_global.ckpointlib == NULL && tmp)
        HYD_handle.user_global.ckpointlib = HYDU_strdup(tmp);
    if (HYD_handle.user_global.ckpointlib == NULL && strcmp(HYDRA_DEFAULT_CKPOINTLIB, ""))
        HYD_handle.user_global.ckpointlib = HYDU_strdup(HYDRA_DEFAULT_CKPOINTLIB);
    if (HYD_handle.user_global.ckpointlib == NULL)
        HYD_handle.user_global.ckpointlib = HYDU_strdup("none");

    tmp = getenv("HYDRA_CKPOINT_PREFIX");
    if (HYD_handle.user_global.ckpoint_prefix == NULL && tmp)
        HYD_handle.user_global.ckpoint_prefix = HYDU_strdup(tmp);

    /* Check environment for setting the inherited environment */
    tmp = getenv("HYDRA_ENV");
    if (HYD_handle.user_global.global_env.prop == NULL && tmp)
        HYD_handle.user_global.global_env.prop =
            !strcmp(tmp, "all") ? HYDU_strdup("all") : HYDU_strdup("none");

    /* If no global environment is set, use the default */
    if (HYD_handle.user_global.global_env.prop == NULL)
        HYD_handle.user_global.global_env.prop = HYDU_strdup("all");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uii_mpx_get_parameters(char **t_argv)
{
    int i;
    char **argv = t_argv;
    char *progname = *argv;
    struct HYD_exec_info *exec_info;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_uiu_init_params();

    status = HYDU_list_inherited_env(&HYD_handle.user_global.global_env.inherited);
    HYDU_ERR_POP(status, "unable to get the inherited env list\n");

    argv++;
    do {
        /* Get the mpiexec arguments  */
        while (*argv && **argv == '-') {
            if (IS_HELP(*argv)) {
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "");
            }

            status = match_arg(&argv);
            HYDU_ERR_POP(status, "argument matching returned error\n");
        }

        if (!(*argv))
            break;

        /* Get the executable information */
        /* Read the executable till you hit the end of a ":" */
        do {
            status = HYD_uiu_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            if (!strcmp(*argv, ":")) {  /* Next executable */
                status = HYDU_alloc_exec_info(&exec_info->next);
                HYDU_ERR_POP(status, "allocate_exec_info returned error\n");
                ++argv;
                break;
            }

            i = 0;
            while (exec_info->exec[i] != NULL)
                i++;
            exec_info->exec[i] = HYDU_strdup(*argv);
            exec_info->exec[i + 1] = NULL;
        } while (++argv && *argv);
    } while (1);

    status = verify_arguments();
    HYDU_ERR_POP(status, "argument verification failed\n");

    /* Get the base path for the proxy */
    if (HYD_handle.user_global.wdir == NULL) {
        HYD_handle.user_global.wdir = HYDU_getcwd();
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.wdir == NULL, HYD_INTERNAL_ERROR,
                            "unable to get current working directory\n");
    }
    status = HYDU_get_base_path(progname, HYD_handle.user_global.wdir, &HYD_handle.base_path);
    HYDU_ERR_POP(status, "unable to get base path\n");

    status = set_default_values();
    HYDU_ERR_POP(status, "setting default values failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
