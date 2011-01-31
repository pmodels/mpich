/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "mpiexec.h"
#include "ui.h"
#include "uiu.h"

static int hostname_propagation = -1;

static void init_ui_mpich_info(void)
{
    HYD_ui_mpich_info.ppn = -1;
    HYD_ui_mpich_info.ckpoint_int = -1;
    HYD_ui_mpich_info.print_rank_map = -1;
    HYD_ui_mpich_info.print_all_exitcodes = -1;
    HYD_ui_mpich_info.ranks_per_proc = -1;
    HYD_ui_mpich_info.sort_order = NONE;
}

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

    HYDU_append_env_to_list(env_name, env_value, &HYD_server_info.user_global.global_env.user);

    if (str[0])
        HYDU_FREE(str[0]);
    if (str[1])
        HYDU_FREE(str[1]);
    if (env_name)
        HYDU_FREE(env_name);
    if (env_value)
        HYDU_FREE(env_value);

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

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC(HYD_server_info.user_global.global_env.prop, char *, len, status);
    HYDU_snprintf(HYD_server_info.user_global.global_env.prop, len, "list:%s", **argv);
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
    return HYDU_set_str(arg, argv, &HYD_server_info.user_global.global_env.prop, "none");
}

static void genvall_help_fn(void)
{
    printf("\n");
    printf("-genvall: All of the ENV is passed to each process (default)\n\n");
    dump_env_notes();
}

static HYD_status genvall_fn(char *arg, char ***argv)
{
    return HYDU_set_str(arg, argv, &HYD_server_info.user_global.global_env.prop, "all");
}

static void mfile_help_fn(void)
{
    printf("\n");
    printf("-f: Host file containing compute node names\n\n");
}

static HYD_status mfile_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.node_list, HYD_INTERNAL_ERROR,
                        "duplicate host file setting\n");

    if (strcmp(**argv, "HYDRA_USE_LOCALHOST")) {
        status =
            HYDU_parse_hostfile(**argv, &HYD_server_info.node_list, HYDU_process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }
    else {
        status = HYDU_add_to_node_list("localhost", 1, &HYD_server_info.node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");
    }

    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void hostlist_help_fn(void)
{
    printf("\n");
    printf("-hostlist: Comma separated list of hosts to run on\n\n");
}

static HYD_status hostlist_fn(char *arg, char ***argv)
{
    char *hostlist[HYD_NUM_TMP_STRINGS];
    int count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.node_list, HYD_INTERNAL_ERROR,
                        "duplicate host file or host list setting\n");

    hostlist[count] = strtok(**argv, ",");
    while ((count < HYD_NUM_TMP_STRINGS) && hostlist[count])
        hostlist[++count] = strtok(NULL, ",");

    if (count >= HYD_NUM_TMP_STRINGS)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "too many hosts listed\n");

    for (count = 0; hostlist[count]; count++) {
        char *h, *procs = NULL;
        int np;

        h = strtok(hostlist[count], ":");
        procs = strtok(NULL, ":");
        np = procs ? atoi(procs) : 1;

        status = HYDU_add_to_node_list(h, np, &HYD_server_info.node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");
    }

    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void ppn_help_fn(void)
{
    printf("\n");
    printf("-ppn: Processes per node\n\n");
}

static HYD_status ppn_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_ui_mpich_info.ppn);
}

static void profile_help_fn(void)
{
    printf("\n");
    printf("-profile: Turn on internal profiling\n\n");
}

static HYD_status profile_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

#if !defined ENABLE_PROFILING
    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "profiling support not compiled in\n");
#endif /* ENABLE_PROFILING */

    status = HYDU_set_int(arg, argv, &HYD_server_info.enable_profiling, 1);
    HYDU_ERR_POP(status, "error enabling profiling\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void prepend_rank_help_fn(void)
{
    printf("\n");
    printf("-prepend-rank: Prepend process rank to stdout and stderr\n\n");
}

static HYD_status prepend_rank_fn(char *arg, char ***argv)
{
    return HYDU_set_str(arg, argv, &HYD_ui_info.prepend_pattern, "[%r] ");
}

static void pattern_info(void)
{
    printf("   Regular expressions can include:\n");
    printf("       %%r: Process rank\n");
    printf("       %%g: Process group ID\n");
    printf("       %%p: Proxy ID\n");
    printf("       %%h: Hostname\n");
}

static void prepend_pattern_help_fn(void)
{
    printf("\n");
    printf("-prepend-pattern: Prepend this regular expression to stdout and stderr\n");
    pattern_info();
    printf("\n");
}

static HYD_status prepend_pattern_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_ui_info.prepend_pattern);
}

static void outfile_pattern_help_fn(void)
{
    printf("\n");
    printf("-outfile-pattern: Send stdout to this file\n\n");
    pattern_info();
}

static HYD_status outfile_pattern_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_ui_info.outfile_pattern);
}

static void errfile_pattern_help_fn(void)
{
    printf("\n");
    printf("-errfile-pattern: Send stderr to this file\n\n");
    pattern_info();
}

static HYD_status errfile_pattern_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_ui_info.errfile_pattern);
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

static struct HYD_arg_match_table match_table[];

static char **config_argv = NULL;

static HYD_status parse_config_argv(void)
{
    struct HYD_exec *exec;
    int i;
    HYD_status status = HYD_SUCCESS;

    do {
        /* Get the mpiexec arguments  */
        status = HYDU_parse_array(&config_argv, match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        if (!(*config_argv))
            break;

        /* Get the executable information */
        do {
            status = get_current_exec(&exec);
            HYDU_ERR_POP(status, "get_current_exec returned error\n");

            i = 0;
            while (exec->exec[i] != NULL)
                i++;
            exec->exec[i] = HYDU_strdup(*config_argv);
            exec->exec[i + 1] = NULL;
        } while (++config_argv && *config_argv);
    } while (1);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status process_config_token(char *token, int newline, struct HYD_node **node_list)
{
    struct HYD_exec *exec;
    static int first_line = 1, idx = 0;
    HYD_status status = HYD_SUCCESS;

    if (config_argv == NULL)
        HYDU_MALLOC(config_argv, char **, HYD_NUM_TMP_STRINGS * sizeof(char *), status);

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    if (newline) {      /* The first entry gives the hostname and processes */
        if (first_line)
            first_line = 0;
        else {
            status = parse_config_argv();
            HYDU_ERR_POP(status, "error parsing config args\n");

            status = HYDU_alloc_exec(&exec->next);
            HYDU_ERR_POP(status, "allocate_exec returned error\n");
            exec->next->appnum = exec->appnum + 1;
        }

        idx = 0;
    }

    config_argv[idx++] = HYDU_strdup(token);
    config_argv[idx] = NULL;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void config_help_fn(void)
{
    printf("\n");
    printf("-configfile: Configuration file for MPMD launch argument information\n\n");
    printf("Notes:\n");
    printf("  * The config file contains information very similar to a command-line\n");
    printf("    launch, except ':' is replaced with a new line character\n");
}

static HYD_status config_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_parse_hostfile(**argv, NULL, process_config_token);
    HYDU_ERR_POP(status, "error parsing config file\n");

    status = parse_config_argv();
    HYDU_ERR_POP(status, "error parsing config args\n");

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

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    HYDU_append_env_to_list(env_name, env_value, &exec->user_env);

    if (str[0])
        HYDU_FREE(str[0]);
    if (str[1])
        HYDU_FREE(str[1]);
    if (env_name)
        HYDU_FREE(env_name);
    if (env_value)
        HYDU_FREE(env_value);

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

    status = HYDU_set_str(arg, argv, &exec->env_prop, "none");

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

    status = HYDU_set_str(arg, argv, &exec->env_prop, "all");

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

    HYD_server_info.pg_list.pg_process_count += exec->proc_count;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void launcher_help_fn(void)
{
    printf("\n");
    printf("-launcher: Launcher to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status launcher_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.launcher);
}

static void launcher_exec_help_fn(void)
{
    printf("\n");
    printf("-launcher-exec: Launcher executable to use\n\n");
    printf("Notes:\n");
    printf("  * This is needed only if Hydra cannot automatically find it\n\n");
}

static HYD_status launcher_exec_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.launcher_exec);
}

static void enablex_help_fn(void)
{
    printf("\n");
    printf("-enable-x: Enable X forwarding\n");
    printf("-disable-x: Disable X forwarding\n\n");
}

static HYD_status enablex_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_server_info.user_global.enablex,
                        !strcmp(arg, "enable-x"));
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
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.rmk);
}

static void ranks_per_proc_help_fn(void)
{
    printf("\n");
    printf("-ranks-per-proc: MPI ranks to assign per launched process\n\n");
}

static HYD_status ranks_per_proc_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_ui_mpich_info.ranks_per_proc);
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
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.binding);
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
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.bindlib);
}

static void ckpoint_interval_help_fn(void)
{
    printf("\n");
    printf("-ckpoint-interval: Checkpointing interval\n\n");
}

static HYD_status ckpoint_interval_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_ui_mpich_info.ckpoint_int);
}

static void ckpoint_prefix_help_fn(void)
{
    printf("\n");
    printf("-ckpoint-prefix: Checkpoint file prefix to use\n\n");
}

static HYD_status ckpoint_prefix_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.ckpoint_prefix);
}

static void ckpoint_num_help_fn(void)
{
    printf("\n");
    printf("-ckpoint-num: Which checkpoint number to restart from.\n\n");
}

static HYD_status ckpoint_num_fn(char *arg, char ***argv)
{
    return HYDU_set_int_and_incr(arg, argv, &HYD_server_info.user_global.ckpoint_num);
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
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.ckpointlib);
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
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.demux);
}

static void verbose_help_fn(void)
{
    printf("\n");
    printf("-verbose: Prints additional debug information\n\n");
}

static HYD_status verbose_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_server_info.user_global.debug, 1);
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
    HYDU_dump_noprefix(stdout,
                       "    Release Date:                            %s\n",
                       HYDRA_RELEASE_DATE);
    HYDU_dump_noprefix(stdout, "    CC:                              %s\n", HYDRA_CC);
    HYDU_dump_noprefix(stdout, "    CXX:                             %s\n", HYDRA_CXX);
    HYDU_dump_noprefix(stdout, "    F77:                             %s\n", HYDRA_F77);
    HYDU_dump_noprefix(stdout, "    F90:                             %s\n", HYDRA_F90);
    HYDU_dump_noprefix(stdout,
                       "    Configure options:                       %s\n",
                       HYDRA_CONFIGURE_ARGS_CLEAN);
    HYDU_dump_noprefix(stdout, "    Process Manager:                         pmi\n");
    HYDU_dump_noprefix(stdout,
                       "    Launchers available:                     %s\n",
                       HYDRA_AVAILABLE_LAUNCHERS);
    HYDU_dump_noprefix(stdout,
                       "    Binding libraries available:             %s\n",
                       HYDRA_AVAILABLE_BINDLIBS);
    HYDU_dump_noprefix(stdout,
                       "    Resource management kernels available:   %s\n",
                       HYDRA_AVAILABLE_RMKS);
    HYDU_dump_noprefix(stdout,
                       "    Checkpointing libraries available:       %s\n",
                       HYDRA_AVAILABLE_CKPOINTLIBS);
    HYDU_dump_noprefix(stdout,
                       "    Demux engines available:                 %s\n",
                       HYDRA_AVAILABLE_DEMUXES);

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
    return HYDU_set_int(arg, argv, &HYD_ui_mpich_info.print_rank_map, 1);
}

static void print_all_exitcodes_help_fn(void)
{
    printf("\n");
    printf("-print-all-exitcodes: Print termination codes of all processes\n\n");
}

static HYD_status print_all_exitcodes_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_ui_mpich_info.print_all_exitcodes, 1);
}

static void iface_help_fn(void)
{
    printf("\n");
    printf("-iface: Network interface to use (e.g., eth0, eth1, myri0, ib0)\n\n");
}

static HYD_status iface_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.user_global.iface);
}

static void nameserver_help_fn(void)
{
    printf("\n");
    printf("-nameserver: Nameserver to use for publish/lookup (format is host:port)\n\n");
}

static HYD_status nameserver_fn(char *arg, char ***argv)
{
    return HYDU_set_str_and_incr(arg, argv, &HYD_server_info.nameserver);
}

static void auto_cleanup_help_fn(void)
{
    printf("\n");
    printf("-disable-auto-cleanup: Don't auto-cleanup of processes when the app aborts\n");
    printf("-enable-auto-cleanup: Auto-cleanup processes when the app aborts (default)\n\n");
}

static HYD_status auto_cleanup_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &HYD_server_info.user_global.auto_cleanup,
                        !strcmp(arg, "enable-auto-cleanup"));
}

static void hostname_propagation_help_fn(void)
{
    printf("\n");
    printf("-disable-hostname-propagation: Let MPICH2 auto-detect the hostname\n");
    printf("-enable-hostname-propagation: Pass user hostnames to MPICH2 (default)\n\n");
}

static HYD_status hostname_propagation_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, argv, &hostname_propagation,
                        strcmp(arg, "disable-hostname-propagation"));
}

static void order_nodes_help_fn(void)
{
    printf("\n");
    printf("-order-nodes ASCENDING: Sort the node list in ascending order\n");
    printf("-order-nodes DESCENDING: Sort the node list in descending order\n");
}

static HYD_status order_nodes_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (!strcmp(**argv, "ASCENDING") || !strcmp(**argv, "ascending") ||
        !strcmp(**argv, "UP") || !strcmp(**argv, "up"))
        HYD_ui_mpich_info.sort_order = ASCENDING;
    else if (!strcmp(**argv, "DESCENDING") || !strcmp(**argv, "descending") ||
             !strcmp(**argv, "DOWN") || !strcmp(**argv, "down"))
        HYD_ui_mpich_info.sort_order = DESCENDING;
    else
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized sort order\n");

    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_arg_match_table match_table[] = {
    /* Global environment options */
    {"genv", genv_fn, genv_help_fn},
    {"genvlist", genvlist_fn, genvlist_help_fn},
    {"genvnone", genvnone_fn, genvnone_help_fn},
    {"genvall", genvall_fn, genvall_help_fn},

    /* Other global options */
    {"f", mfile_fn, mfile_help_fn},
    {"hostfile", mfile_fn, mfile_help_fn},
    {"machinefile", mfile_fn, mfile_help_fn},
    {"machine", hostlist_fn, mfile_help_fn},
    {"machines", hostlist_fn, mfile_help_fn},
    {"machinelist", hostlist_fn, mfile_help_fn},
    {"host", hostlist_fn, hostlist_help_fn},
    {"hosts", hostlist_fn, hostlist_help_fn},
    {"hostlist", hostlist_fn, hostlist_help_fn},
    {"ppn", ppn_fn, ppn_help_fn},
    {"profile", profile_fn, profile_help_fn},
    {"prepend-rank", prepend_rank_fn, prepend_rank_help_fn},
    {"l", prepend_rank_fn, prepend_rank_help_fn},
    {"prepend-pattern", prepend_pattern_fn, prepend_pattern_help_fn},
    {"outfile-pattern", outfile_pattern_fn, outfile_pattern_help_fn},
    {"errfile-pattern", errfile_pattern_fn, errfile_pattern_help_fn},
    {"outfile", outfile_pattern_fn, outfile_pattern_help_fn},
    {"errfile", errfile_pattern_fn, errfile_pattern_help_fn},
    {"wdir", wdir_fn, wdir_help_fn},
    {"configfile", config_fn, config_help_fn},

    /* Local environment options */
    {"env", env_fn, env_help_fn},
    {"envlist", envlist_fn, envlist_help_fn},
    {"envnone", envnone_fn, envnone_help_fn},
    {"envall", envall_fn, envall_help_fn},

    /* Other local options */
    {"n", np_fn, np_help_fn},
    {"np", np_fn, np_help_fn},

    /* Hydra specific options */

    /* Launcher options */
    {"launcher", launcher_fn, launcher_help_fn},
    {"launcher-exec", launcher_exec_fn, launcher_exec_help_fn},
    {"bootstrap", launcher_fn, launcher_help_fn},
    {"bootstrap-exec", launcher_exec_fn, launcher_exec_help_fn},
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
    {"ckpoint-num", ckpoint_num_fn, ckpoint_num_help_fn},
    {"ckpointlib", ckpointlib_fn, ckpointlib_help_fn},

    /* Demux engine options */
    {"demux", demux_fn, demux_help_fn},

    /* Other hydra options */
    {"verbose", verbose_fn, verbose_help_fn},
    {"v", verbose_fn, verbose_help_fn},
    {"debug", verbose_fn, verbose_help_fn},
    {"info", info_fn, info_help_fn},
    {"version", info_fn, info_help_fn},
    {"print-rank-map", print_rank_map_fn, print_rank_map_help_fn},
    {"print-all-exitcodes", print_all_exitcodes_fn, print_all_exitcodes_help_fn},
    {"iface", iface_fn, iface_help_fn},
    {"nameserver", nameserver_fn, nameserver_help_fn},
    {"disable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"enable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"disable-hostname-propagation", hostname_propagation_fn, hostname_propagation_help_fn},
    {"enable-hostname-propagation", hostname_propagation_fn, hostname_propagation_help_fn},
    {"order-nodes", order_nodes_fn, order_nodes_help_fn},

    {"\0", NULL}
};

static HYD_status set_default_values(void)
{
    char *tmp;
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    /* If exec_list is not NULL, make sure local executable is set */
    for (exec = HYD_uii_mpx_exec_list; exec; exec = exec->next) {
        if (exec->exec[0] == NULL && HYD_server_info.user_global.ckpoint_prefix == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        status = HYDU_correct_wdir(&exec->wdir);
        HYDU_ERR_POP(status, "unable to correct wdir\n");
    }

    if (HYD_ui_mpich_info.print_rank_map == -1)
        HYD_ui_mpich_info.print_rank_map = 0;

    if (HYD_ui_mpich_info.print_all_exitcodes == -1)
        HYD_ui_mpich_info.print_all_exitcodes = 0;

    if (HYD_ui_mpich_info.ranks_per_proc == -1)
        HYD_ui_mpich_info.ranks_per_proc = 1;

    if (HYD_server_info.enable_profiling == -1)
        HYD_server_info.enable_profiling = 0;

    if (HYD_server_info.user_global.debug == -1 &&
        MPL_env2bool("HYDRA_DEBUG", &HYD_server_info.user_global.debug) == 0)
        HYD_server_info.user_global.debug = 0;

    HYD_GET_ENV_STR_VAL(HYD_server_info.user_global.iface, "HYDRA_IFACE", NULL);

    if (HYD_server_info.node_list == NULL &&
        MPL_env2str("HYDRA_HOST_FILE", (const char **) &tmp)) {
        status =
            HYDU_parse_hostfile(tmp, &HYD_server_info.node_list, HYDU_process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }

    /* Check environment for setting the inherited environment */
    if (HYD_server_info.user_global.global_env.prop == NULL &&
        MPL_env2str("HYDRA_ENV", (const char **) &tmp))
        HYD_server_info.user_global.global_env.prop =
            !strcmp(tmp, "all") ? HYDU_strdup("all") : HYDU_strdup("none");

    if (HYD_server_info.user_global.auto_cleanup == -1)
        HYD_server_info.user_global.auto_cleanup = 1;

    /* Make sure this is either a restart or there is an executable to
     * launch */
    if (HYD_uii_mpx_exec_list == NULL && HYD_server_info.user_global.ckpoint_prefix == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable provided\n");

    if (hostname_propagation || hostname_propagation == -1)
        HYD_server_info.interface_env_name = HYDU_strdup("MPICH_INTERFACE_HOSTNAME");

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
    init_ui_mpich_info();

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
        HYD_server_info.base_path = NULL;
        status = HYDU_find_in_path(progname, &HYD_server_info.base_path);
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
            status = HYDU_str_alloc_and_join(tmp, &HYD_server_info.base_path);
            HYDU_ERR_POP(status, "unable to join strings\n");
            HYDU_free_strlist(tmp);
        }
        else {  /* absolute */
            HYD_server_info.base_path = HYDU_strdup(post);
        }
    }
    HYDU_FREE(post);

    status = set_default_values();
    HYDU_ERR_POP(status, "setting default values failed\n");

    /* If the user set the checkpoint prefix, set env var to enable
     * checkpointing on the processes  */
    if (HYD_server_info.user_global.ckpoint_prefix)
        HYDU_append_env_to_list("MPICH_ENABLE_CKPOINT", "1",
                                &HYD_server_info.user_global.global_env.system);

    /* Preset common environment options for disabling STDIO buffering
     * in Fortran */
    HYDU_append_env_to_list("GFORTRAN_UNBUFFERED_PRECONNECTED", "y",
                            &HYD_server_info.user_global.global_env.system);

    /* If auto-cleanup is disabled, ask MPICH2 to enabled
     * FT-collective returns */
    if (HYD_server_info.user_global.auto_cleanup == 0) {
        HYDU_append_env_to_list("MPICH_ENABLE_COLL_FT_RET", "1",
                                &HYD_server_info.user_global.global_env.system);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
