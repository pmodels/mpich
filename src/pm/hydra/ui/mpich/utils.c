/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
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
static char *config_file = NULL;
static char **config_argv = NULL;
static int reading_config_file = 0;
static struct HYD_arg_match_table match_table[];

static void init_ui_mpich_info(void)
{
    HYD_ui_mpich_info.ppn = -1;
    HYD_ui_mpich_info.print_all_exitcodes = -1;
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

static void help_help_fn(void)
{
    printf("\n");
    printf
        ("Usage: ./mpiexec [global opts] [local opts for exec1] [exec1] [exec1 args] : [local opts for exec2] [exec2] [exec2 args] : ...\n\n");

    printf("Global options (passed to all executables):\n");

    printf("\n");
    printf("  Global environment options:\n");
    printf("    -genv {name} {value}             environment variable name and value\n");
    printf("    -genvlist {env1,env2,...}        environment variable list to pass\n");
    printf("    -genvnone                        do not pass any environment variables\n");
    printf("    -genvall                         pass all environment variables not managed\n");
    printf("                                          by the launcher (default)\n");

    printf("\n");
    printf("  Other global options:\n");
    printf("    -f {name}                        file containing the host names\n");
    printf("    -hosts {host list}               comma separated host list\n");
    printf("    -wdir {dirname}                  working directory to use\n");
    printf("    -configfile {name}               config file containing MPMD launch options\n");

    printf("\n");
    printf("\n");

    printf("Local options (passed to individual executables):\n");

    printf("\n");
    printf("  Local environment options:\n");
    printf("    -env {name} {value}              environment variable name and value\n");
    printf("    -envlist {env1,env2,...}         environment variable list to pass\n");
    printf("    -envnone                         do not pass any environment variables\n");
    printf("    -envall                          pass all environment variables (default)\n");

    printf("\n");
    printf("  Other local options:\n");
    printf("    -n/-np {value}                   number of processes\n");
    printf("    {exec_name} {args}               executable name and arguments\n");

    printf("\n");
    printf("\n");

    printf("Hydra specific options (treated as global):\n");

    printf("\n");
    printf("  Launch options:\n");
    printf("    -launcher                        launcher to use (%s)\n",
           HYDRA_AVAILABLE_LAUNCHERS);
    printf("    -launcher-exec                   executable to use to launch processes\n");
    printf("    -enable-x/-disable-x             enable or disable X forwarding\n");

    printf("\n");
    printf("  Resource management kernel options:\n");
    printf("    -rmk                             resource management kernel to use (%s)\n",
           HYDRA_AVAILABLE_RMKS);

    printf("\n");
    printf("  Processor topology options:\n");
    printf("    -topolib                         processor topology library (%s)\n",
           HYDRA_AVAILABLE_TOPOLIBS);
    printf("    -bind-to                         process binding\n");
    printf("    -map-by                          process mapping\n");
    printf("    -membind                         memory binding policy\n");

    printf("\n");
    printf("  Demux engine options:\n");
    printf("    -demux                           demux engine (%s)\n", HYDRA_AVAILABLE_DEMUXES);

    printf("\n");
    printf("  Other Hydra options:\n");
    printf("    -verbose                         verbose mode\n");
    printf("    -info                            build information\n");
    printf("    -print-all-exitcodes             print exit codes of all processes\n");
    printf("    -iface                           network interface to use\n");
    printf("    -ppn                             processes per node\n");
    printf("    -profile                         turn on internal profiling\n");
    printf("    -prepend-rank                    prepend rank to output\n");
    printf("    -prepend-pattern                 prepend pattern to output\n");
    printf("    -outfile-pattern                 direct stdout to file\n");
    printf("    -errfile-pattern                 direct stderr to file\n");
    printf("    -nameserver                      name server information (host:port format)\n");
    printf("    -disable-auto-cleanup            don't cleanup processes on error\n");
    printf("    -disable-hostname-propagation    let MPICH auto-detect the hostname\n");
    printf("    -order-nodes                     order nodes as ascending/descending cores\n");
    printf("    -localhost                       local hostname for the launching node\n");
    printf("    -usize                           universe size (SYSTEM, INFINITE, <value>)\n");

    printf("\n");
    printf("Please see the intructions provided at\n");
    printf("http://wiki.mpich.org/mpich/index.php/Using_the_Hydra_Process_Manager\n");
    printf("for further details\n\n");
}

static HYD_status help_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    help_help_fn();
    HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "%s", "");

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

    env_name = MPL_strdup(str[0]);
    if (str[1] == NULL) {       /* The environment is not of the form x=foo */
        if (**argv == NULL) {
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
        env_value = MPL_strdup(**argv);
        (*argv)++;
    }
    else {
        env_value = MPL_strdup(str[1]);
    }

    HYDU_append_env_to_list(env_name, env_value, &HYD_server_info.user_global.global_env.user);

    if (str[0])
        MPL_free(str[0]);
    if (str[1])
        MPL_free(str[1]);
    if (env_name)
        MPL_free(env_name);
    if (env_value)
        MPL_free(env_value);

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
    size_t len;
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.global_env.prop) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC_OR_JUMP(HYD_server_info.user_global.global_env.prop, char *, len, status);
    MPL_snprintf(HYD_server_info.user_global.global_env.prop, len, "list:%s", **argv);

  fn_exit:
    (*argv)++;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.global_env.prop) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.global_env.prop, "none");
    HYDU_ERR_POP(status, "error setting genvnone\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void genvall_help_fn(void)
{
    printf("\n");
    printf("-genvall: All of the ENV is passed to each process (default)\n\n");
    dump_env_notes();
}

static HYD_status genvall_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.global_env.prop) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.global_env.prop, "all");
    HYDU_ERR_POP(status, "error setting genvall\n");

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
    char localhost[MAX_HOSTNAME_LEN] = { 0 };
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.node_list) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.node_list, HYD_INTERNAL_ERROR,
                        "duplicate host file setting\n");

    if (strcmp(**argv, "HYDRA_USE_LOCALHOST")) {
        status = HYDU_parse_hostfile(**argv, &HYD_server_info.node_list, HYDU_process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }
    else {
        if (gethostname(localhost, MAX_HOSTNAME_LEN) < 0)
            HYDU_ERR_SETANDJUMP(status, HYD_SOCK_ERROR, "unable to get local hostname\n");

        status = HYDU_add_to_node_list(localhost, 1, &HYD_server_info.node_list);
        HYDU_ERR_POP(status, "unable to add to node list\n");
    }

  fn_exit:
    (*argv)++;
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
    char *hostlist[HYD_NUM_TMP_STRINGS + 1];    /* +1 for null termination of list */
    int count = 0;
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.node_list) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

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

  fn_exit:
    (*argv)++;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_ui_mpich_info.ppn != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &HYD_ui_mpich_info.ppn, atoi(**argv));
    HYDU_ERR_POP(status, "error setting ppn\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void profile_help_fn(void)
{
    printf("\n");
    printf("-profile: Turn on internal profiling\n\n");
}

static HYD_status profile_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.enable_profiling != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

#if !defined ENABLE_PROFILING
    HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "profiling support not compiled in\n");
#endif /* ENABLE_PROFILING */

    status = HYDU_set_int(arg, &HYD_server_info.enable_profiling, 1);
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_ui_info.prepend_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_info.prepend_pattern, "[%r] ");
    HYDU_ERR_POP(status, "error setting prepend_rank field\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_ui_info.prepend_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_info.prepend_pattern, **argv);
    HYDU_ERR_POP(status, "error setting prepend pattern\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void outfile_pattern_help_fn(void)
{
    printf("\n");
    printf("-outfile-pattern: Send stdout to this file\n\n");
    pattern_info();
}

static HYD_status outfile_pattern_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_ui_info.outfile_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_info.outfile_pattern, **argv);
    HYDU_ERR_POP(status, "error setting outfile pattern\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void errfile_pattern_help_fn(void)
{
    printf("\n");
    printf("-errfile-pattern: Send stderr to this file\n\n");
    pattern_info();
}

static HYD_status errfile_pattern_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_ui_info.errfile_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_info.errfile_pattern, **argv);
    HYDU_ERR_POP(status, "error setting errfile pattern\n");

  fn_exit:
    (*argv)++;
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

    status = HYDU_set_str(arg, &exec->wdir, **argv);
    HYDU_ERR_POP(status, "error setting wdir for executable\n");

  fn_exit:
    (*argv)++;
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

    HYDU_ASSERT(!config_file, status);
    config_file = MPL_strdup(**argv);
    (*argv)++;

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

    env_name = MPL_strdup(str[0]);
    if (str[1] == NULL) {       /* The environment is not of the form x=foo */
        if (**argv == NULL) {
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
        env_value = MPL_strdup(**argv);
        (*argv)++;
    }
    else {
        env_value = MPL_strdup(str[1]);
    }

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    HYDU_append_env_to_list(env_name, env_value, &exec->user_env);

    if (str[0])
        MPL_free(str[0]);
    if (str[1])
        MPL_free(str[1]);
    if (env_name)
        MPL_free(env_name);
    if (env_value)
        MPL_free(env_value);

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
    size_t len;
    struct HYD_exec *exec;
    HYD_status status = HYD_SUCCESS;

    status = get_current_exec(&exec);
    HYDU_ERR_POP(status, "get_current_exec returned error\n");

    HYDU_ERR_CHKANDJUMP(status, exec->env_prop, HYD_INTERNAL_ERROR,
                        "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC_OR_JUMP(exec->env_prop, char *, len, status);
    MPL_snprintf(exec->env_prop, len, "list:%s", **argv);
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

    status = HYDU_set_str(arg, &exec->env_prop, "none");

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

    status = HYDU_set_str(arg, &exec->env_prop, "all");

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

    status = HYDU_set_int(arg, &exec->proc_count, atoi(**argv));
    HYDU_ERR_POP(status, "error getting executable process count\n");

  fn_exit:
    (*argv)++;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.launcher) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.launcher, **argv);
    HYDU_ERR_POP(status, "error setting launcher\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.launcher_exec) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.launcher_exec, **argv);
    HYDU_ERR_POP(status, "error setting launcher exec\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void enablex_help_fn(void)
{
    printf("\n");
    printf("-enable-x: Enable X forwarding\n");
    printf("-disable-x: Disable X forwarding\n\n");
}

static HYD_status enablex_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.enablex != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &HYD_server_info.user_global.enablex, !strcmp(arg, "enable-x"));
    HYDU_ERR_POP(status, "error setting enable-x\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.rmk) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.rmk, **argv);
    HYDU_ERR_POP(status, "error setting rmk\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void bind_to_help_fn(void)
{
    printf("\n");
    printf("-bind-to: Process-core binding type to use\n\n");
    printf("    Binding type options:\n");
    printf("        Default:\n");
    printf("            none             -- no binding (default)\n");
    printf("\n");
    printf("        Architecture unaware options:\n");
    printf("            rr               -- round-robin as OS assigned processor IDs\n");
    printf("            user:0+2,1+4,3,2 -- user specified binding\n");
    printf("\n");
    printf("        Architecture aware options (part within the {} braces are optional):\n");
    printf("            board{:<n>}      -- bind to 'n' motherboards\n");
    printf("            numa{:<n>}       -- bind to 'n' numa domains\n");
    printf("            socket{:<n>}     -- bind to 'n' sockets\n");
    printf("            core{:<n>}       -- bind to 'n' cores\n");
    printf("            hwthread{:<n>}   -- bind to 'n' hardware threads\n");
    printf("            l1cache{:<n>}    -- bind to processes on 'n' L1 cache domains\n");
    printf("            l2cache{:<n>}    -- bind to processes on 'n' L2 cache domains\n");
    printf("            l3cache{:<n>}    -- bind to processes on 'n' L3 cache domains\n");

    printf("\n\n");

    printf("-map-by: Order of bind mapping to use\n\n");
    printf("    Options (T: hwthread; C: core; S: socket; N: NUMA domain; B: motherboard):\n");
    printf("        Default: <same option as binding>\n");
    printf("\n");
    printf("        Architecture aware options:\n");
    printf("            board            -- map to motherboard\n");
    printf("            numa             -- map to numa domain\n");
    printf("            socket           -- map to socket\n");
    printf("            core             -- map to core\n");
    printf("            hwthread         -- map to hardware thread\n");
    printf("            l1cache          -- map to L1 cache domain\n");
    printf("            l2cache          -- map to L2 cache domain\n");
    printf("            l3cache          -- map to L3 cache domain\n");

    printf("\n\n");

    printf("-membind: Memory binding policy\n\n");
    printf("    Memory binding policy options:\n");
    printf("        Default:\n");
    printf("            none             -- no binding (default)\n");
    printf("\n");
    printf("        Architecture aware options:\n");
    printf("            firsttouch        -- closest to process that first touches memory\n");
    printf("            nexttouch         -- closest to process that next touches memory\n");
    printf("            bind:<list>       -- bind to memory node list\n");
    printf("            interleave:<list> -- interleave among memory node list\n");
    printf("            replicate:<list>  -- replicate among memory node list\n");
}

static HYD_status bind_to_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.binding) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.binding, **argv);
    HYDU_ERR_POP(status, "error setting binding\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status map_by_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.mapping) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.mapping, **argv);
    HYDU_ERR_POP(status, "error setting mapping\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status membind_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.membind) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.membind, **argv);
    HYDU_ERR_POP(status, "error setting mapping\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void topolib_help_fn(void)
{
    printf("\n");
    printf("-topolib: Topology library to use\n\n");
    printf("Notes:\n");
    printf("  * Use the -info option to see what all are compiled in\n\n");
}

static HYD_status topolib_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.topolib) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.topolib, **argv);
    HYDU_ERR_POP(status, "error setting topolib\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
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
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.demux) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.demux, **argv);
    HYDU_ERR_POP(status, "error setting demux\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void verbose_help_fn(void)
{
    printf("\n");
    printf("-verbose: Prints additional debug information\n\n");
}

static HYD_status verbose_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.debug != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &HYD_server_info.user_global.debug, 1);
    HYDU_ERR_POP(status, "error setting debug\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
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
    HYDU_dump_noprefix(stdout, "    Version:                                 %s\n", HYDRA_VERSION);
    HYDU_dump_noprefix(stdout,
                       "    Release Date:                            %s\n", HYDRA_RELEASE_DATE);
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
                       "    Topology libraries available:            %s\n",
                       HYDRA_AVAILABLE_TOPOLIBS);
    HYDU_dump_noprefix(stdout,
                       "    Resource management kernels available:   %s\n", HYDRA_AVAILABLE_RMKS);
    HYDU_dump_noprefix(stdout,
                       "    Demux engines available:                 %s\n",
                       HYDRA_AVAILABLE_DEMUXES);

    HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "%s", "");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void print_all_exitcodes_help_fn(void)
{
    printf("\n");
    printf("-print-all-exitcodes: Print termination codes of all processes\n\n");
}

static HYD_status print_all_exitcodes_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_ui_mpich_info.print_all_exitcodes != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &HYD_ui_mpich_info.print_all_exitcodes, 1);
    HYDU_ERR_POP(status, "error setting print_all_exitcodes\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void iface_help_fn(void)
{
    printf("\n");
    printf("-iface: Network interface to use (e.g., eth0, eth1, myri0, ib0)\n\n");
}

static HYD_status iface_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.iface) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.user_global.iface, **argv);
    HYDU_ERR_POP(status, "error setting iface\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void nameserver_help_fn(void)
{
    printf("\n");
    printf("-nameserver: Nameserver to use for publish/lookup (format is host:port)\n\n");
}

static HYD_status nameserver_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.nameserver) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.nameserver, **argv);
    HYDU_ERR_POP(status, "error setting nameserver\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void auto_cleanup_help_fn(void)
{
    printf("\n");
    printf("-disable-auto-cleanup: Don't auto-cleanup of processes when the app aborts\n");
    printf("-enable-auto-cleanup: Auto-cleanup processes when the app aborts (default)\n\n");
}

static HYD_status auto_cleanup_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.auto_cleanup != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &HYD_server_info.user_global.auto_cleanup,
                          !strcmp(arg, "enable-auto-cleanup"));
    HYDU_ERR_POP(status, "error setting auto cleanup\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void hostname_propagation_help_fn(void)
{
    printf("\n");
    printf("-disable-hostname-propagation: Let MPICH auto-detect the hostname\n");
    printf("-enable-hostname-propagation: Pass user hostnames to MPICH (default)\n\n");
}

static HYD_status hostname_propagation_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && hostname_propagation != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &hostname_propagation, strcmp(arg, "disable-hostname-propagation"));
    HYDU_ERR_POP(status, "error setting hostname propagation\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
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

    if (reading_config_file && HYD_ui_mpich_info.sort_order != NONE) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    if (!strcmp(**argv, "ASCENDING") || !strcmp(**argv, "ascending") ||
        !strcmp(**argv, "UP") || !strcmp(**argv, "up"))
        HYD_ui_mpich_info.sort_order = ASCENDING;
    else if (!strcmp(**argv, "DESCENDING") || !strcmp(**argv, "descending") ||
             !strcmp(**argv, "DOWN") || !strcmp(**argv, "down"))
        HYD_ui_mpich_info.sort_order = DESCENDING;
    else
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized sort order\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void localhost_help_fn(void)
{
    printf("\n");
    printf("-localhost: Local hostname to use for the launching node\n\n");
}

static HYD_status localhost_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.localhost) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_server_info.localhost, **argv);
    HYDU_ERR_POP(status, "error setting local hostname\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void usize_help_fn(void)
{
    printf("\n");
    printf("-usize: Universe size (SYSTEM, INFINITE, <value>\n");
    printf("   SYSTEM: Number of cores passed to mpiexec through hostfile or resource manager\n");
    printf("   INFINITE: No limit\n");
    printf("   <value>: Numeric value >= 0\n\n");
}

static HYD_status usize_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (reading_config_file && HYD_server_info.user_global.usize) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.usize != HYD_USIZE_UNSET,
                        HYD_INTERNAL_ERROR, "universe size already set\n");

    if (!strcmp(**argv, "SYSTEM"))
        HYD_server_info.user_global.usize = HYD_USIZE_SYSTEM;
    else if (!strcmp(**argv, "INFINITE"))
        HYD_server_info.user_global.usize = HYD_USIZE_INFINITE;
    else {
        HYD_server_info.user_global.usize = atoi(**argv);
        HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.usize <= 0,
                            HYD_INTERNAL_ERROR, "invalid universe size\n");
    }

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

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

    if (HYD_ui_mpich_info.print_all_exitcodes == -1)
        HYD_ui_mpich_info.print_all_exitcodes = 0;

    if (HYD_server_info.enable_profiling == -1)
        HYD_server_info.enable_profiling = 0;

    if (HYD_server_info.user_global.debug == -1 &&
        MPL_env2bool("HYDRA_DEBUG", &HYD_server_info.user_global.debug) == 0)
        HYD_server_info.user_global.debug = 0;

    /* don't clobber existing iface values from the command line */
    if (HYD_server_info.user_global.iface == NULL) {
        if (MPL_env2str("HYDRA_IFACE", (const char **) &tmp) != 0)
            HYD_server_info.user_global.iface = MPL_strdup(tmp);
        tmp = NULL;
    }

    if (HYD_server_info.node_list == NULL && MPL_env2str("HYDRA_HOST_FILE", (const char **) &tmp)) {
        status = HYDU_parse_hostfile(tmp, &HYD_server_info.node_list, HYDU_process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    }

    /* Check environment for setting the inherited environment */
    if (HYD_server_info.user_global.global_env.prop == NULL &&
        MPL_env2str("HYDRA_ENV", (const char **) &tmp))
        HYD_server_info.user_global.global_env.prop =
            !strcmp(tmp, "all") ? MPL_strdup("all") : MPL_strdup("none");

    if (HYD_server_info.user_global.auto_cleanup == -1)
        HYD_server_info.user_global.auto_cleanup = 1;

    /* If hostname propagation is not set on the command-line, check
     * for the environment variable */
    if (hostname_propagation == -1)
        MPL_env2bool("HYDRA_HOSTNAME_PROPAGATION", &hostname_propagation);

    /* If an interface is provided, set that */
    if (HYD_server_info.user_global.iface) {
        if (hostname_propagation == 1) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "cannot set iface and force hostname propagation");
        }

        HYDU_append_env_to_list("MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE",
                                HYD_server_info.user_global.iface,
                                &HYD_server_info.user_global.global_env.system);

        /* Disable hostname propagation */
        hostname_propagation = 0;
    }

    /* If hostname propagation is requested (or not set), set the
     * environment variable for doing that */
    if (hostname_propagation || hostname_propagation == -1)
        HYD_server_info.iface_ip_env_name = MPL_strdup("MPIR_CVAR_CH3_INTERFACE_HOSTNAME");

    /* Default universe size if the user did not specify anything is
     * INFINITE */
    if (HYD_server_info.user_global.usize == HYD_USIZE_UNSET)
        HYD_server_info.user_global.usize = HYD_USIZE_INFINITE;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status process_config_token(char *token, int newline, struct HYD_node **node_list)
{
    static int idx = 0;

    if (idx && newline && strcmp(config_argv[idx - 1], ":")) {
        /* If this is a newline, but not the first one, and the
         * previous token was not a ":", add an executable delimiter
         * ':' */
        config_argv[idx++] = MPL_strdup(":");
    }

    config_argv[idx++] = MPL_strdup(token);
    config_argv[idx] = NULL;

    return HYD_SUCCESS;
}

static HYD_status parse_args(char **t_argv)
{
    char **argv = t_argv;
    struct HYD_exec *exec;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

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
            exec->exec[i] = MPL_strdup(*argv);
            exec->exec[i + 1] = NULL;
        } while (++argv && *argv);
    } while (1);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uii_mpx_get_parameters(char **t_argv)
{
    int ret;
    size_t len;
    char **argv = t_argv;
    char *progname = *argv;
    char *post, *loc, *tmp[HYD_NUM_TMP_STRINGS], *conf_file;
    const char *home, *env_file;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_uiu_init_params();
    init_ui_mpich_info();

    argv++;
    status = parse_args(argv);
    HYDU_ERR_POP(status, "unable to parse user arguments\n");

    if (config_file == NULL) {
        /* Check if there's a config file location in the environment */
        ret = MPL_env2str("HYDRA_CONFIG_FILE", &env_file);
        if (ret) {
            ret = open(env_file, O_RDONLY);
            if (ret >= 0) {
                close(ret);
                config_file = MPL_strdup(env_file);
                goto config_file_check_exit;
            }
        }

        /* Check if there's a config file in the home directory */
        ret = MPL_env2str("HOME", &home);
        if (ret) {
            len = strlen(home) + strlen("/.mpiexec.hydra.conf") + 1;

            HYDU_MALLOC_OR_JUMP(conf_file, char *, len, status);
            MPL_snprintf(conf_file, len, "%s/.mpiexec.hydra.conf", home);

            ret = open(conf_file, O_RDONLY);
            if (ret < 0) {
                MPL_free(conf_file);
            }
            else {
                close(ret);
                config_file = conf_file;
                goto config_file_check_exit;
            }
        }

        /* Check if there's a config file in the hard-coded location */
        conf_file = MPL_strdup(HYDRA_CONF_FILE);
        ret = open(conf_file, O_RDONLY);
        if (ret < 0) {
            MPL_free(conf_file);
        }
        else {
            close(ret);
            config_file = conf_file;
            goto config_file_check_exit;
        }
    }

  config_file_check_exit:
    if (config_file) {
        HYDU_ASSERT(config_argv == NULL, status);
        HYDU_MALLOC_OR_JUMP(config_argv, char **, HYD_NUM_TMP_STRINGS * sizeof(char *), status);

        status = HYDU_parse_hostfile(config_file, NULL, process_config_token);
        HYDU_ERR_POP(status, "error parsing config file\n");

        reading_config_file = 1;
        status = parse_args(config_argv);
        HYDU_ERR_POP(status, "error parsing config args\n");
        reading_config_file = 0;

        MPL_free(config_file);
    }

    /* Get the base path */
    /* Find the last '/' in the executable name */
    post = MPL_strdup(progname);
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
            tmp[1] = MPL_strdup("/");
            tmp[2] = MPL_strdup(post);
            tmp[3] = NULL;
            status = HYDU_str_alloc_and_join(tmp, &HYD_server_info.base_path);
            HYDU_ERR_POP(status, "unable to join strings\n");
            HYDU_free_strlist(tmp);
        }
        else {  /* absolute */
            HYD_server_info.base_path = MPL_strdup(post);
        }
    }
    MPL_free(post);

    status = set_default_values();
    HYDU_ERR_POP(status, "setting default values failed\n");

    /* Preset common environment options for disabling STDIO buffering
     * in Fortran */
    HYDU_append_env_to_list("GFORTRAN_UNBUFFERED_PRECONNECTED", "y",
                            &HYD_server_info.user_global.global_env.system);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_arg_match_table match_table[] = {
    /* help options */
    {"help", help_fn, help_help_fn},
    {"h", help_fn, help_help_fn},

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

    /* Topology options */
    {"topolib", topolib_fn, topolib_help_fn},
    {"binding", bind_to_fn, bind_to_help_fn},
    {"bind-to", bind_to_fn, bind_to_help_fn},
    {"map-by", map_by_fn, bind_to_help_fn},
    {"membind", membind_fn, bind_to_help_fn},

    /* Demux engine options */
    {"demux", demux_fn, demux_help_fn},

    /* Other hydra options */
    {"verbose", verbose_fn, verbose_help_fn},
    {"v", verbose_fn, verbose_help_fn},
    {"debug", verbose_fn, verbose_help_fn},
    {"info", info_fn, info_help_fn},
    {"version", info_fn, info_help_fn},
    {"print-all-exitcodes", print_all_exitcodes_fn, print_all_exitcodes_help_fn},
    {"iface", iface_fn, iface_help_fn},
    {"nameserver", nameserver_fn, nameserver_help_fn},
    {"disable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"dac", auto_cleanup_fn, auto_cleanup_help_fn},
    {"enable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"disable-hostname-propagation", hostname_propagation_fn, hostname_propagation_help_fn},
    {"enable-hostname-propagation", hostname_propagation_fn, hostname_propagation_help_fn},
    {"order-nodes", order_nodes_fn, order_nodes_help_fn},
    {"localhost", localhost_fn, localhost_help_fn},
    {"usize", usize_fn, usize_help_fn},

    {"\0", NULL}
};
