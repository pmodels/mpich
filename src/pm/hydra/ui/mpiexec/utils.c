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

HYD_Handle HYD_handle;

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

static HYD_Status genv_fn(char *arg, char ***argv)
{
    char *env_name, *env_value, *str[2] = { 0 };
    HYD_Env_t *env;
    HYD_Status status = HYD_SUCCESS;

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

    HYDU_append_env_to_list(*env, &HYD_handle.global_env.user);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status genvlist_fn(char *arg, char ***argv)
{
    int len;
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.global_env.prop,
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
    HYDU_MALLOC(HYD_handle.global_env.prop, char *, len, status);
    HYDU_snprintf(HYD_handle.global_env.prop, len, "list:%s", **argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status genvnone_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-genvnone: None of the ENV is passed to each process\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.global_env.prop = HYDU_strdup("none");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status genvall_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-genvall: All of the ENV is passed to each process (default)\n\n");
        dump_env_notes();
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.global_env.prop = HYDU_strdup("all");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_UII_mpx_init_proxy_list(char *hostname, int num_procs)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_alloc_proxy(&HYD_handle.proxy_list);
    HYDU_ERR_POP(status, "unable to allocate proxy\n");

    HYD_handle.proxy_list->hostname = HYDU_strdup(hostname);

    status = HYDU_alloc_proxy_segment(&HYD_handle.proxy_list->segment_list);
    HYDU_ERR_POP(status, "unable to allocate proxy segment\n");

    HYD_handle.proxy_list->segment_list->start_pid = 0;
    HYD_handle.proxy_list->segment_list->proc_count = num_procs;

    HYD_handle.proxy_list->proxy_core_count += num_procs;
    HYD_handle.global_core_count += num_procs;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status process_mfile_token(char *token, int newline)
{
    static int pid = 0;
    int num_procs;
    char *hostname, *procs;
    struct HYD_Proxy *proxy;
    struct HYD_Proxy_segment *proxy_segment;
    HYD_Status status = HYD_SUCCESS;

    if (newline) { /* The first entry gives the hostname and processes */
        hostname = strtok(token, ":");
        procs = strtok(NULL, ":");
        num_procs = procs ? atoi(procs) : 1;

        if (HYD_handle.proxy_list == NULL) {
            status = HYD_UII_mpx_init_proxy_list(hostname, num_procs);
            HYDU_ERR_POP(status, "unable to initialize proxy\n");
        }
        else {
            for (proxy = HYD_handle.proxy_list;
                 proxy->next && strcmp(proxy->hostname, hostname); proxy = proxy->next);

            if (strcmp(proxy->hostname, hostname)) {
                /* If the hostname does not match, create a new proxy */
                status = HYDU_alloc_proxy(&proxy->next);
                HYDU_ERR_POP(status, "unable to allocate proxy\n");

                proxy = proxy->next;
            }

            for (proxy_segment = proxy->segment_list; proxy_segment->next;
                 proxy_segment = proxy_segment->next);

            /* If this segment is a continuation, just increment the
             * size of the previous segment */
            if (proxy_segment->start_pid + proxy_segment->proc_count == pid)
                proxy_segment->proc_count += num_procs;
            else {
                status = HYDU_alloc_proxy_segment(&proxy_segment->next);
                HYDU_ERR_POP(status, "unable to allocate proxy segment\n");

                proxy_segment->next->start_pid = pid;
                proxy_segment->next->proc_count = num_procs;
            }

            HYD_handle.proxy_list->proxy_core_count += num_procs;
            HYD_handle.global_core_count += num_procs;
        }

        pid += num_procs;
    }
    else { /* Not a new line */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "token %s not supported at this time\n", token);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status mfile_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.proxy_list, HYD_INTERNAL_ERROR,
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
        status = HYD_UII_mpx_init_proxy_list((char *) "localhost", 1);
        HYDU_ERR_POP(status, "unable to initialize proxy\n");
    }

    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status wdir_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.wdir, HYD_INTERNAL_ERROR,
                        "duplicate wdir setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-wdir: Working directory to use on the compute nodes\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.wdir = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status env_fn(char *arg, char ***argv)
{
    char *env_name, *env_value, *str[2] = { 0 };
    HYD_Env_t *env;
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

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

    status = HYD_UIU_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    HYDU_append_env_to_list(*env, &exec_info->user_env);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status envlist_fn(char *arg, char ***argv)
{
    int len;
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    status = HYD_UIU_get_current_exec_info(&exec_info);
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

static HYD_Status envnone_fn(char *arg, char ***argv)
{
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    status = HYD_UIU_get_current_exec_info(&exec_info);
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

static HYD_Status envall_fn(char *arg, char ***argv)
{
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    status = HYD_UIU_get_current_exec_info(&exec_info);
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

static HYD_Status np_fn(char *arg, char ***argv)
{
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-n: Number of processes to launch\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    status = HYD_UIU_get_current_exec_info(&exec_info);
    HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

    if (exec_info->process_count != 0)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate process count\n");

    exec_info->process_count = atoi(**argv);
    HYD_handle.global_process_count += exec_info->process_count;
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status bootstrap_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.bootstrap, HYD_INTERNAL_ERROR,
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

    HYD_handle.bootstrap = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status bootstrap_exec_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.bootstrap_exec, HYD_INTERNAL_ERROR,
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

    HYD_handle.bootstrap_exec = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status enablex_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.enablex != -1, HYD_INTERNAL_ERROR,
                        "duplicate -enable-x argument\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-enable-x: Enable X forwarding\n");
        printf("-disable-x: Disable X forwarding\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (!strcmp(arg, "enable-x"))
        HYD_handle.enablex = 1;
    else
        HYD_handle.enablex = 0;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status boot_proxies_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.launch_mode != HYD_LAUNCH_UNSET,
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
        HYD_handle.launch_mode = HYD_LAUNCH_BOOT;
    else if (!strcmp(arg, "boot-foreground-proxies"))
        HYD_handle.launch_mode = HYD_LAUNCH_BOOT_FOREGROUND;
    else if (!strcmp(arg, "shutdown-proxies"))
        HYD_handle.launch_mode = HYD_LAUNCH_SHUTDOWN;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status proxy_port_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status use_persistent_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.launch_mode != HYD_LAUNCH_UNSET,
                        HYD_INTERNAL_ERROR, "duplicate launch mode\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-use-persistent: Use persistent proxies\n");
        printf("-use-runtime: Launch proxies at runtime\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    if (!strcmp(arg, "use-persistent"))
        HYD_handle.launch_mode = HYD_LAUNCH_PERSISTENT;
    else
        HYD_handle.launch_mode = HYD_LAUNCH_RUNTIME;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status css_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status rmk_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status ranks_per_proc_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status enable_pm_env_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status binding_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.binding, HYD_INTERNAL_ERROR,
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
        printf("        buddy -- order of least shared resources\n");
        printf("        pack  -- order of most shared resources\n\n");
        printf("        user:0,1,3,2 -- user specified binding\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.binding = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status bindlib_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.bindlib, HYD_INTERNAL_ERROR,
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

    HYD_handle.bindlib = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status ckpoint_interval_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status ckpoint_prefix_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.ckpoint_prefix, HYD_INTERNAL_ERROR,
                        "duplicate -ckpoint-prefix option\n");

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no ckpoint prefix specified\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ckpoint-prefix: Checkpoint file prefix to use\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.ckpoint_prefix = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status ckpointlib_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.ckpointlib, HYD_INTERNAL_ERROR,
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

    HYD_handle.ckpointlib = HYDU_strdup(**argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status ckpoint_restart_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.ckpoint_restart, HYD_INTERNAL_ERROR, "duplicate restart mode\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-ckpoint-restart: Restart checkpointed application\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.ckpoint_restart = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status verbose_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.debug != -1, HYD_INTERNAL_ERROR,
                        "duplicate verbose setting\n");

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-verbose: Prints additional debug information\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    HYD_handle.debug = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status info_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

    if (**argv && IS_HELP(**argv)) {
        printf("\n");
        printf("-info: Shows build information\n\n");
        HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
    }

    printf("HYDRA build details:\n");
    printf("    Process Manager: pmi\n");
    printf("    Boot-strap servers available: %s\n", HYDRA_BSS_NAMES);
    printf("    Communication sub-systems available: none\n");
    printf("    Binding libraries available: %s\n", HYDRA_BINDLIB_NAMES);
    printf("    Checkpointing libraries available: %s\n", HYDRA_CKPOINTLIB_NAMES);

    HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_Status print_rank_map_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status print_all_exitcodes_fn(char *arg, char ***argv)
{
    HYD_Status status = HYD_SUCCESS;

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
     HYD_Status(*handler) (char *arg, char ***argv_p);
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

static HYD_Status match_arg(char ***argv_p)
{
    struct match_table_fns *m;
    char *arg, *tmp;
    HYD_Status status = HYD_SUCCESS;

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

static HYD_Status verify_arguments(void)
{
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    /* Proxy launch or checkpoint restart */
    if ((HYD_handle.launch_mode == HYD_LAUNCH_BOOT) ||
        (HYD_handle.launch_mode == HYD_LAUNCH_BOOT_FOREGROUND) ||
        (HYD_handle.launch_mode == HYD_LAUNCH_SHUTDOWN)) {

        /* No environment */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.global_env.prop, HYD_INTERNAL_ERROR,
                            "env setting not required for booting/shutting proxies\n");

        /* No binding */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.binding, HYD_INTERNAL_ERROR,
                            "binding not allowed while booting/shutting proxies\n");
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.bindlib, HYD_INTERNAL_ERROR,
                            "binding not allowed while booting/shutting proxies\n");

        /* No executables */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.exec_info_list, HYD_INTERNAL_ERROR,
                            "execs should not be specified while booting/shutting proxies\n");
    }
    else {      /* Application launch */
        /* On a checkpoint restart, we set the prefix as the application */
        if (HYD_handle.ckpoint_restart == 1) {
            status = HYD_UIU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            exec_info->exec[0] = HYDU_strdup(HYD_handle.ckpoint_prefix);
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

static HYD_Status set_default_values(void)
{
    char *tmp;
    HYD_Status status = HYD_SUCCESS;

    if (HYD_handle.print_rank_map == -1)
        HYD_handle.print_rank_map = 0;

    if (HYD_handle.print_all_exitcodes == -1)
        HYD_handle.print_all_exitcodes = 0;

    tmp = getenv("HYDRA_DEBUG");
    if (HYD_handle.debug == -1 && tmp)
        HYD_handle.debug = atoi(tmp) ? 1 : 0;
    if (HYD_handle.debug == -1)
        HYD_handle.debug = 0;

    tmp = getenv("HYDRA_PM_ENV");
    if (HYD_handle.pm_env == -1 && tmp)
        HYD_handle.pm_env = (atoi(getenv("HYDRA_PM_ENV")) != 0);
    if (HYD_handle.pm_env == -1)
        HYD_handle.pm_env = 1;  /* Default is to pass the PM environment */

    tmp = getenv("HYDRA_BOOTSTRAP");
    if (HYD_handle.bootstrap == NULL && tmp)
        HYD_handle.bootstrap = HYDU_strdup(tmp);
    if (HYD_handle.bootstrap == NULL)
        HYD_handle.bootstrap = HYDU_strdup(HYDRA_DEFAULT_BSS);

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
    if (HYD_handle.proxy_list == NULL && tmp) {
        status = HYDU_parse_hostfile(tmp, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }

    tmp = getenv("HYDRA_PROXY_PORT");
    if (HYD_handle.proxy_port == -1 && tmp)
        HYD_handle.proxy_port = atoi(getenv("HYDRA_PROXY_PORT"));
    if (HYD_handle.proxy_port == -1)
        HYD_handle.proxy_port = HYD_DEFAULT_PROXY_PORT;

    tmp = getenv("HYDRA_LAUNCH_MODE");
    if (HYD_handle.launch_mode == HYD_LAUNCH_UNSET && tmp) {
        if (!strcmp(tmp, "persistent"))
            HYD_handle.launch_mode = HYD_LAUNCH_PERSISTENT;
        else if (!strcmp(tmp, "runtime"))
            HYD_handle.launch_mode = HYD_LAUNCH_RUNTIME;
    }
    if (HYD_handle.launch_mode == HYD_LAUNCH_UNSET)
        HYD_handle.launch_mode = HYD_LAUNCH_RUNTIME;

    tmp = getenv("HYDRA_BOOT_FOREGROUND_PROXIES");
    if (HYD_handle.launch_mode == HYD_LAUNCH_UNSET && tmp) {
        if (atoi(tmp) == 1) {
            HYD_handle.launch_mode = HYD_LAUNCH_BOOT_FOREGROUND;
        }
    }

    tmp = getenv("HYDRA_BOOTSTRAP_EXEC");
    if (HYD_handle.bootstrap_exec == NULL && tmp)
        HYD_handle.bootstrap_exec = HYDU_strdup(tmp);

    /* Check environment for setting binding */
    tmp = getenv("HYDRA_BINDING");
    if (HYD_handle.binding == NULL && tmp)
        HYD_handle.binding = HYDU_strdup(tmp);

    tmp = getenv("HYDRA_BINDLIB");
    if (HYD_handle.bindlib == NULL && tmp)
        HYD_handle.bindlib = HYDU_strdup(tmp);
    if (HYD_handle.bindlib == NULL && strcmp(HYDRA_DEFAULT_BINDLIB, ""))
        HYD_handle.bindlib = HYDU_strdup(HYDRA_DEFAULT_BINDLIB);
    if (HYD_handle.bindlib == NULL)
        HYD_handle.bindlib = HYDU_strdup("none");

    /* Check environment for checkpointing */
    tmp = getenv("HYDRA_CKPOINTLIB");
    if (HYD_handle.ckpointlib == NULL && tmp)
        HYD_handle.ckpointlib = HYDU_strdup(tmp);
    if (HYD_handle.ckpointlib == NULL && strcmp(HYDRA_DEFAULT_CKPOINTLIB, ""))
        HYD_handle.ckpointlib = HYDU_strdup(HYDRA_DEFAULT_CKPOINTLIB);
    if (HYD_handle.ckpointlib == NULL)
        HYD_handle.ckpointlib = HYDU_strdup("none");

    tmp = getenv("HYDRA_CKPOINT_PREFIX");
    if (HYD_handle.ckpoint_prefix == NULL && tmp)
        HYD_handle.ckpoint_prefix = HYDU_strdup(tmp);

    /* Check environment for setting the inherited environment */
    tmp = getenv("HYDRA_ENV");
    if (HYD_handle.global_env.prop == NULL && tmp)
        HYD_handle.global_env.prop =
            !strcmp(tmp, "all") ? HYDU_strdup("all") : HYDU_strdup("none");

    /* If no global environment is set, use the default */
    if (HYD_handle.global_env.prop == NULL)
        HYD_handle.global_env.prop = HYDU_strdup("all");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_Status HYD_UII_mpx_get_parameters(char **t_argv)
{
    int i;
    char **argv = t_argv;
    char *progname = *argv;
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_UIU_init_params();

    status = HYDU_list_inherited_env(&HYD_handle.global_env.inherited);
    HYDU_ERR_POP(status, "unable to get the inherited env list\n");

    argv++;
    do {
        /* Get the mpiexec arguments  */
        while (*argv && **argv == '-') {
            status = match_arg(&argv);
            HYDU_ERR_POP(status, "argument matching returned error\n");
        }

        if (!(*argv))
            break;

        /* Get the executable information */
        /* Read the executable till you hit the end of a ":" */
        do {
            status = HYD_UIU_get_current_exec_info(&exec_info);
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
    if (HYD_handle.wdir == NULL) {
        HYD_handle.wdir = HYDU_getcwd();
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.wdir == NULL, HYD_INTERNAL_ERROR,
                            "unable to get current working directory\n");
    }
    status = HYDU_get_base_path(progname, HYD_handle.wdir, &HYD_handle.base_path);
    HYDU_ERR_POP(status, "unable to get base path\n");

    status = set_default_values();
    HYDU_ERR_POP(status, "setting default values failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
