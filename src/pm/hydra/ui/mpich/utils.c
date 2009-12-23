/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "uiu.h"

static HYD_status get_current_exec(struct HYD_exec **exec)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_uii_mpx_exec_list == NULL) {
        status = HYDU_alloc_exec(&HYD_uii_mpx_exec_list);
        HYDU_ERR_POP(status, "unable to allocate exec\n");
        HYD_uii_mpx_exec_list->appnum = 0;
    }

    *exec = HYD_uii_mpx_exec_list;
    while ((*exec)->next)
        *exec = (*exec)->next;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

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

static void genv_help_fn(void)
{
    printf("\n");
    printf("-genv: Specify an ENV-VALUE pair to be passed to each process\n\n");
    printf("Notes:\n");
    printf("  * Usage: -genv env1 val1\n");
    printf("  * Can be used multiple times for different variables\n\n");
    dump_env_notes();
}

static HYD_status genv_fn(char *arg, char ***argv)
{
    char *env_name, *env_value, *str[2] = { 0 };
    struct HYD_env *env;
    HYD_status status = HYD_SUCCESS;

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

static void genvlist_help_fn(void)
{
    printf("\n");
    printf("-genvlist: A list of ENV variables is passed to each process\n\n");
    printf("Notes:\n");
    printf("  * Usage: -genvlist env1,env2,env2\n");
    printf("  * Values of env1,env2,env3 are taken from the user ENV\n\n");
    dump_env_notes();
}

static HYD_status genvlist_fn(char *arg, char ***argv)
{
    int len;
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC(HYD_handle.user_global.global_env.prop, char *, len, status);
    HYDU_snprintf(HYD_handle.user_global.global_env.prop, len, "list:%s", **argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void genvnone_help_fn(void)
{
    printf("\n");
    printf("-genvnone: None of the ENV is passed to each process\n\n");
    dump_env_notes();
}

static HYD_status genvnone_fn(char *arg, char ***argv)
{
    return HYDU_set_str(arg, argv, &HYD_handle.user_global.global_env.prop, (char *) "none");
}

static void genvall_help_fn(void)
{
    printf("\n");
    printf("-genvall: All of the ENV is passed to each process (default)\n\n");
    dump_env_notes();
}

static HYD_status genvall_fn(char *arg, char ***argv)
{
    return HYDU_set_str(arg, argv, &HYD_handle.user_global.global_env.prop, (char *) "all");
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
        HYDU_ERR_POP(status, "unable to add to node list\n");
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

static void mfile_help_fn(void)
{
    printf("\n");
    printf("-f: Host file containing compute node names\n\n");
}

static HYD_status mfile_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_handle.node_list, HYD_INTERNAL_ERROR,
                        "duplicate host file setting\n");

    if (strcmp(**argv, "HYDRA_USE_LOCALHOST")) {
        status = HYDU_parse_hostfile(**argv, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }
    else {
        status = HYDU_add_to_node_list((char *) "localhost", 1, &HYD_handle.node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");
    }

    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void wdir_help_fn(void)
{
    printf("\n");
    printf("-wdir: Working directory to use on the compute nodes\n\n");
}

static HYD_status wdir_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    status = HYDU_set_str_and_incr(arg, argv, &exec->wdir);
    HYDU_ERR_POP(status, "error setting wdir for executable\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void env_help_fn(void)
{
    printf("\n");
    printf("-env: Specify an ENV-VALUE pair to be passed to this executable\n\n");
    printf("Notes:\n");
    printf("  * Usage: -env env1 val1\n");
    printf("  * Can be used multiple times for different variables\n\n");
    dump_env_notes();
}

static HYD_status env_fn(char *arg, char ***argv)
{
    char *env_name, *env_value, *str[2] = { 0 };
    struct HYD_env *env;
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

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

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    HYDU_append_env_to_list(*env, &exec->user_env);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void envlist_help_fn(void)
{
    printf("\n");
    printf("-envlist: A list of ENV variables is passed to this executable\n\n");
    printf("Notes:\n");
    printf("  * Usage: -envlist env1,env2,env2\n");
    printf("  * Values of env1,env2,env3 are taken from the user ENV\n\n");
    dump_env_notes();
}

static HYD_status envlist_fn(char *arg, char ***argv)
{
    int len;
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    HYDU_ERR_CHKANDJUMP(status, exec->env_prop, HYD_INTERNAL_ERROR,
                        "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC(exec->env_prop, char *, len, status);
    HYDU_snprintf(exec->env_prop, len, "list:%s", **argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void envnone_help_fn(void)
{
    printf("\n");
    printf("-envnone: None of the ENV is passed to this executable\n\n");
    dump_env_notes();
}

static HYD_status envnone_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    status = HYDU_set_str(arg, argv, &exec->env_prop, (char *) "none");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void envall_help_fn(void)
{
    printf("\n");
    printf("-envall: All of the ENV is passed to this executable\n\n");
    dump_env_notes();
}

static HYD_status envall_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    status = HYDU_set_str(arg, argv, &exec->env_prop, (char *) "all");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void np_help_fn(void)
{
    printf("\n");
    printf("-n: Number of processes to launch\n\n");
}

static HYD_status np_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    status = HYDU_set_int_and_incr(arg, argv, &exec->proc_count);
    HYDU_ERR_POP(status, "error getting executable process count\n");

    HYD_handle.pg_list.pg_process_count += exec->proc_count;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void bootstrap_help_fn(void)
{
    printf("\n");
    printf("-bootstrap: Bootstrap server to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status bootstrap_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.bootstrap);
}

static void bootstrap_exec_help_fn(void)
{
    printf("\n");
    printf("-bootstrap-exec: Bootstrap executable to use\n\n");
    printf("Notes:\n");
    printf("  * This is needed only if Hydra cannot automatically find it\n\n");
}

static HYD_status bootstrap_exec_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.bootstrap_exec);
}

static void enablex_help_fn(void)
{
    printf("\n");
    printf("-enable-x: Enable X forwarding\n");
    printf("-disable-x: Disable X forwarding\n\n");
}

static HYD_status enablex_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_handle.user_global.enablex, !strcmp(arg, "enable-x"));
}

static void rmk_help_fn(void)
{
    printf("\n");
    printf("-rmk: Resource management kernel to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status rmk_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.rmk);
}

static void ranks_per_proc_help_fn(void)
{
    printf("\n");
    printf("-ranks-per-proc: MPI ranks to assign per launched process\n\n");
}

static HYD_status ranks_per_proc_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_handle.ranks_per_proc);
}

static void binding_help_fn(void)
{
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
}

static HYD_status binding_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.binding);
}

static void bindlib_help_fn(void)
{
    printf("\n");
    printf("-bindlib: Binding library to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status bindlib_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.bindlib);
}

static void ckpoint_interval_help_fn(void)
{
    printf("\n");
    printf("-ckpoint-interval: Checkpointing interval\n\n");
}

static HYD_status ckpoint_interval_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_handle.ckpoint_int);
}

static void ckpoint_prefix_help_fn(void)
{
    printf("\n");
    printf("-ckpoint-prefix: Checkpoint file prefix to use\n\n");
}

static HYD_status ckpoint_prefix_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.ckpoint_prefix);
}

static void ckpointlib_help_fn(void)
{
    printf("\n");
    printf("-ckpointlib: Checkpointing library to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status ckpointlib_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.ckpointlib);
}

static void demux_help_fn(void)
{
    printf("\n");
    printf("-demux: Demux engine to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status demux_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_handle.user_global.demux);
}

static void verbose_help_fn(void)
{
    printf("\n");
    printf("-verbose: Prints additional debug information\n\n");
}

static HYD_status verbose_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_handle.user_global.debug, 1);
}

static void info_help_fn(void)
{
    printf("\n");
    printf("-info: Shows build information\n\n");
}

static HYD_status info_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

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
                       "    Bootstrap servers available:             %s\n", HYDRA_BSS_NAMES);
    HYDU_dump_noprefix(stdout, "    Communication sub-systems available:     none\n");
    HYDU_dump_noprefix(stdout,
                       "    Binding libraries available:             %s\n",
                       HYDRA_BINDLIB_NAMES);
    HYDU_dump_noprefix(stdout,
                       "    Resource management kernels available:   %s\n",
                       HYDRA_RMK_NAMES);
    HYDU_dump_noprefix(stdout,
                       "    Checkpointing libraries available:       %s\n",
                       HYDRA_CKPOINTLIB_NAMES);
    HYDU_dump_noprefix(stdout,
                       "    Demux engines available:                 %s\n",
                       HYDRA_DEMUX_NAMES);

    HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void print_rank_map_help_fn(void)
{
    printf("\n");
    printf("-print-rank-map: Print what ranks are allocated to what nodes\n\n");
}

static HYD_status print_rank_map_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_handle.print_rank_map, 1);
}

static void print_all_exitcodes_help_fn(void)
{
    printf("\n");
    printf("-print-all-exitcodes: Print termination codes of all processes\n\n");
}

static HYD_status print_all_exitcodes_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_handle.print_all_exitcodes, 1);
}

static struct HYD_arg_match_table match_table[] = {
    /* Global environment options */
    {"genv", genv_fn, genv_help_fn},
    {"genvlist", genvlist_fn, genvlist_help_fn},
    {"genvnone", genvnone_fn, genvnone_help_fn},
    {"genvall", genvall_fn, genvall_help_fn},

    /* Other global options */
    {"f", mfile_fn, mfile_help_fn},
    {"host", mfile_fn, mfile_help_fn},
    {"hosts", mfile_fn, mfile_help_fn},
    {"hostfile", mfile_fn, mfile_help_fn},
    {"machine", mfile_fn, mfile_help_fn},
    {"machines", mfile_fn, mfile_help_fn},
    {"machinefile", mfile_fn, mfile_help_fn},
    {"wdir", wdir_fn, wdir_help_fn},

    /* Local environment options */
    {"env", env_fn, env_help_fn},
    {"envlist", envlist_fn, envlist_help_fn},
    {"envnone", envnone_fn, envnone_help_fn},
    {"envall", envall_fn, envall_help_fn},

    /* Other local options */
    {"n", np_fn, np_help_fn},
    {"np", np_fn, np_help_fn},

    /* Hydra specific options */

    /* Bootstrap options */
    {"bootstrap", bootstrap_fn, bootstrap_help_fn},
    {"bootstrap-exec", bootstrap_exec_fn, bootstrap_exec_help_fn},
    {"enable-x", enablex_fn, enablex_help_fn},
    {"disable-x", enablex_fn, enablex_help_fn},

    /* Resource management kernel options */
    {"rmk", rmk_fn, rmk_help_fn},

    /* Hybrid programming options */
    {"ranks-per-proc", ranks_per_proc_fn, ranks_per_proc_help_fn},

    /* Process-core binding options */
    {"binding", binding_fn, binding_help_fn},
    {"bindlib", bindlib_fn, bindlib_help_fn},

    /* Checkpoint/restart options */
    {"ckpoint-interval", ckpoint_interval_fn, ckpoint_interval_help_fn},
    {"ckpoint-prefix", ckpoint_prefix_fn, ckpoint_prefix_help_fn},
    {"ckpointlib", ckpointlib_fn, ckpointlib_help_fn},

    /* Demux engine options */
    {"demux", demux_fn, demux_help_fn},

    /* Other hydra options */
    {"verbose", verbose_fn, verbose_help_fn},
    {"v", verbose_fn, verbose_help_fn},
    {"debug", verbose_fn, verbose_help_fn},
    {"info", info_fn, info_help_fn},
    {"print-rank-map", print_rank_map_fn, print_rank_map_help_fn},
    {"print-all-exitcodes", print_all_exitcodes_fn, print_all_exitcodes_help_fn},

    {"\0", NULL}
};

static HYD_status set_default_values(void)
{
    char *tmp;
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    /* If exec_list is not NULL, make sure local executable is set */
    for (exec = HYD_uii_mpx_exec_list; exec; exec = exec->next) {
        if (exec->exec[0] == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        status = HYDU_correct_wdir(&exec->wdir);
        HYDU_ERR_POP(status, "unable to correct wdir\n");
    }

    if (HYD_handle.print_rank_map == -1)
        HYD_handle.print_rank_map = 0;

    if (HYD_handle.print_all_exitcodes == -1)
        HYD_handle.print_all_exitcodes = 0;

    tmp = getenv("HYDRA_DEBUG");
    if (HYD_handle.user_global.debug == -1 && tmp)
        HYD_handle.user_global.debug = atoi(tmp) ? 1 : 0;
    if (HYD_handle.user_global.debug == -1)
        HYD_handle.user_global.debug = 0;

    tmp = getenv("HYDRA_BOOTSTRAP");
    if (HYD_handle.user_global.bootstrap == NULL && tmp)
        HYD_handle.user_global.bootstrap = HYDU_strdup(tmp);
    if (HYD_handle.user_global.bootstrap == NULL)
        HYD_handle.user_global.bootstrap = HYDU_strdup(HYDRA_DEFAULT_BSS);

    tmp = getenv("HYDRA_RMK");
    if (HYD_handle.rmk == NULL && tmp)
        HYD_handle.rmk = HYDU_strdup(tmp);

    tmp = getenv("HYDRA_DEMUX");
    if (HYD_handle.user_global.demux == NULL && tmp)
        HYD_handle.user_global.demux = HYDU_strdup(tmp);
    if (HYD_handle.user_global.demux == NULL)
        HYD_handle.user_global.demux = HYDU_strdup(HYDRA_DEFAULT_DEMUX);

    tmp = getenv("HYDRA_HOST_FILE");
    if (HYD_handle.node_list == NULL && tmp) {
        status = HYDU_parse_hostfile(tmp, process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
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

    /* Check environment for checkpointing */
    tmp = getenv("HYDRA_CKPOINTLIB");
    if (HYD_handle.user_global.ckpointlib == NULL && tmp)
        HYD_handle.user_global.ckpointlib = HYDU_strdup(tmp);
    if (HYD_handle.user_global.ckpointlib == NULL && strcmp(HYDRA_DEFAULT_CKPOINTLIB, ""))
        HYD_handle.user_global.ckpointlib = HYDU_strdup(HYDRA_DEFAULT_CKPOINTLIB);

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
    char *post, *loc, *tmp[HYD_NUM_TMP_STRINGS];
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_uiu_init_params();

    status = HYDU_list_inherited_env(&HYD_handle.user_global.global_env.inherited);
    HYDU_ERR_POP(status, "unable to get the inherited env list\n");

    argv++;
    do {
        /* Get the mpiexec arguments  */
        status = HYDU_parse_array(&argv, match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        if (!(*argv))
            break;

        /* Get the executable information */
        /* Read the executable till you hit the end of a ":" */
        do {
            status = get_current_exec(&exec);
            HYDU_ERR_POP(status, "get_current_exec returned error\n");

            if (!strcmp(*argv, ":")) {  /* Next executable */
                status = HYDU_alloc_exec(&exec->next);
                HYDU_ERR_POP(status, "allocate_exec returned error\n");
                exec->next->appnum = exec->appnum + 1;
                ++argv;
                break;
            }

            i = 0;
            while (exec->exec[i] != NULL)
                i++;
            exec->exec[i] = HYDU_strdup(*argv);
            exec->exec[i + 1] = NULL;
        } while (++argv && *argv);
    } while (1);

    /* Get the base path */
    /* Find the last '/' in the executable name */
    post = HYDU_strdup(progname);
    loc = strrchr(post, '/');
    if (!loc) { /* If there is no path */
        HYD_handle.base_path = NULL;
        status = HYDU_find_in_path(progname, &HYD_handle.base_path);
        HYDU_ERR_POP(status, "error while searching for executable in the user path\n");
    }
    else {      /* There is a path */
        *(++loc) = 0;

        /* Check if its absolute or relative */
        if (post[0] != '/') {   /* relative */
            tmp[0] = HYDU_getcwd();
            tmp[1] = HYDU_strdup("/");
            tmp[2] = HYDU_strdup(post);
            tmp[3] = NULL;
            status = HYDU_str_alloc_and_join(tmp, &HYD_handle.base_path);
            HYDU_ERR_POP(status, "unable to join strings\n");
            HYDU_free_strlist(tmp);
        }
        else {  /* absolute */
            HYD_handle.base_path = HYDU_strdup(post);
        }
    }
    HYDU_FREE(post);

    status = set_default_values();
    HYDU_ERR_POP(status, "setting default values failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
