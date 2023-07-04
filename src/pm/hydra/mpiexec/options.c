/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "mpiexec.h"
#include "uiu.h"

/* This file defines HYD_mpiexec_match_table, which includes all the
 * argument parsing routines and help messages for command line options.
 */

#define ASSERT_ARGV \
    do { \
        if (!**argv) { \
            status = HYD_FAILURE; \
            HYDU_ERR_POP(status, "missing command line argument, add -h for help\n"); \
        }\
    } while (0)

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
    printf("    -rankmap {rank map}              comma separated rank to node id list\n");
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
    printf("    -disable-auto-cleanup            don't cleanup processes on error\n");
    printf("    -errfile-pattern                 direct stderr to file\n");
    printf("    -gpus-per-proc                   number of GPUs per process (default: auto)\n");
    printf("    -hybrid-hosts                    assume hosts do not share paths\n");
    printf("    -iface                           network interface to use\n");
    printf("    -info                            build information\n");
    printf("    -localhost                       local hostname for the launching node\n");
    printf("    -nameserver                      name server information (host:port format)\n");
    printf("    -order-nodes                     order nodes as ascending/descending cores\n");
    printf("    -outfile-pattern                 direct stdout to file\n");
    printf("    -pmi-port                        use the PMI_PORT model\n");
    printf("    -ppn                             processes per node\n");
    printf("    -prepend-pattern                 prepend pattern to output\n");
    printf("    -prepend-rank                    prepend rank to output\n");
    printf("    -print-all-exitcodes             print exit codes of all processes\n");
    printf("    -profile                         turn on internal profiling\n");
    printf("    -skip-launch-node                do not run MPI processes on the launch node\n");
    printf("    -usize                           universe size (SYSTEM, INFINITE, <value>)\n");
    printf("    -verbose                         verbose mode\n");

    printf("\n");
    printf("Please see the instructions provided at\n");
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
    } else {
        env_value = MPL_strdup(str[1]);
    }

    HYDU_append_env_to_list(env_name, env_value, &HYD_server_info.user_global.global_env.user);

    MPL_free(str[0]);
    MPL_free(str[1]);
    MPL_free(env_name);
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.global_env.prop) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.global_env.prop,
                        HYD_INTERNAL_ERROR, "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC_OR_JUMP(HYD_server_info.user_global.global_env.prop, char *, len, status);
    snprintf(HYD_server_info.user_global.global_env.prop, len, "list:%s", **argv);

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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.global_env.prop) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.global_env.prop) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.node_list) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.node_list, HYD_INTERNAL_ERROR,
                        "duplicate host file setting\n");

    if (strcmp(**argv, "HYDRA_USE_LOCALHOST")) {
        status = HYDU_parse_hostfile(**argv, &HYD_server_info.node_list, HYDU_process_mfile_token);
        HYDU_ERR_POP(status, "error parsing hostfile\n");
    } else {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.node_list) {
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

static void rankmap_help_fn(void)
{
    printf("\n");
    printf("-rankmap: rank to node id mapping\n\n");
    printf("The basic format is a list of node ids, one for each rank. For example\n");
    printf("    1,1,2,2\n");
    printf("puts rank 0 and 1 on node 1, rank 2 and 3 on node 2.\n\n");
    printf("Consecutive nodes with same number of process-per-node can be combined\n");
    printf("using a block syntax \"(start_id, num_nodes, num_ranks)\", for example,\n");
    printf("    (1,2,2)\n");
    printf("describes the same rankmap as\n");
    printf("    1,1,2,2\n");
    printf("\n");
}

static HYD_status rankmap_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (strncmp(**argv, "(vector,", 8) != 0) {
        int n = strlen(**argv);
        char *tmp_str = MPL_malloc(n + 10, MPL_MEM_OTHER);
        snprintf(tmp_str, n + 10, "(vector,%s)", **argv);
        HYD_server_info.rankmap = tmp_str;
    } else {
        HYD_server_info.rankmap = MPL_strdup(**argv);
    }
    HYDU_ASSERT(HYD_server_info.rankmap, status);

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

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.ppn != -1) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.enable_profiling != -1) {
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

static void output_from_help_fn(void)
{
    printf("\n");
    printf("-output-from: only show the output from this rank\n\n");
}

static HYD_status output_from_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.output_from != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_int(arg, &HYD_ui_mpich_info.output_from, atoi(**argv));
    HYDU_ERR_POP(status, "error setting output_from\n");

  fn_exit:
    (*argv)++;
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.prepend_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_mpich_info.prepend_pattern, "[%r] ");
    HYDU_ERR_POP(status, "error setting prepend_rank field\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void pattern_info(void)
{
    printf("   Supported substitutions include:\n");
    printf("       %%r: Process rank (in MPI_COMM_WORLD)\n");
    printf("       %%g: Process group ID\n");
    printf("       %%p: Proxy ID\n");
    printf("       %%h: Hostname\n");
    printf("       %%t: Timestamp in seconds\n");
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.prepend_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_mpich_info.prepend_pattern, **argv);
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.outfile_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_mpich_info.outfile_pattern, **argv);
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.errfile_pattern) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    status = HYDU_set_str(arg, &HYD_ui_mpich_info.errfile_pattern, **argv);
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

    status = HYD_uii_get_current_exec(&exec);
    HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

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

    HYDU_ASSERT(!HYD_ui_mpich_info.config_file, status);
    HYD_ui_mpich_info.config_file = MPL_strdup(**argv);
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
    } else {
        env_value = MPL_strdup(str[1]);
    }

    status = HYD_uii_get_current_exec(&exec);
    HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

    HYDU_append_env_to_list(env_name, env_value, &exec->user_env);

    MPL_free(str[0]);
    MPL_free(str[1]);
    MPL_free(env_name);
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

    status = HYD_uii_get_current_exec(&exec);
    HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

    HYDU_ERR_CHKANDJUMP(status, exec->env_prop, HYD_INTERNAL_ERROR,
                        "duplicate environment setting\n");

    len = strlen("list:") + strlen(**argv) + 1;
    HYDU_MALLOC_OR_JUMP(exec->env_prop, char *, len, status);
    snprintf(exec->env_prop, len, "list:%s", **argv);
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

    status = HYD_uii_get_current_exec(&exec);
    HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

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

    status = HYD_uii_get_current_exec(&exec);
    HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

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

    status = HYD_uii_get_current_exec(&exec);
    HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

    ASSERT_ARGV;
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.launcher) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.launcher_exec) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.enablex != -1) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.rmk) {
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
    printf("            machine          -- bind to machine\n");
    printf("            numa{:<n>}       -- bind to 'n' numa domains\n");
    printf("            socket{:<n>}     -- bind to 'n' sockets\n");
    printf("            core{:<n>}       -- bind to 'n' cores\n");
    printf("            hwthread{:<n>}   -- bind to 'n' hardware threads\n");
    printf("            l1cache{:<n>}    -- bind to 'n' L1 cache domains\n");
    printf("            l1dcache{:<n>}   -- bind to 'n' L1 data cache domain\n");
    printf("            l1icache{:<n>}   -- bind to 'n' L1 instruction cache domain\n");
    printf("            l1ucache{:<n>}   -- bind to 'n' L1 unified cache domain\n");
    printf("            l2cache{:<n>}    -- bind to 'n' L2 cache domains\n");
    printf("            l2dcache{:<n>}   -- bind to 'n' L2 data cache domain\n");
    printf("            l2icache{:<n>}   -- bind to 'n' L2 instruction cache domain\n");
    printf("            l2ucache{:<n>}   -- bind to 'n' L2 unified cache domain\n");
    printf("            l3cache{:<n>}    -- bind to 'n' L3 cache domain\n");
    printf("            l3dcache{:<n>}   -- bind to 'n' L3 data cache domain\n");
    printf("            l3icache{:<n>}   -- bind to 'n' L3 instruction cache domain\n");
    printf("            l3ucache{:<n>}   -- bind to 'n' L3 unified cache domain\n");
    printf("            l4cache{:<n>}    -- bind to 'n' L4 cache domain\n");
    printf("            l4dcache{:<n>}   -- bind to 'n' L4 data cache domain\n");
    printf("            l4ucache{:<n>}   -- bind to 'n' L4 unified cache domain\n");
    printf("            l5cache{:<n>}    -- bind to 'n' L5 cache domain\n");
    printf("            l5dcache{:<n>}   -- bind to 'n' L5 data cache domain\n");
    printf("            l5ucache{:<n>}   -- bind to 'n' L5 unified cache domain\n");
    printf("            pci:<id>         -- bind to non-io ancestor of PCI device\n");
    printf("            gpu{<id>|:<n>}   -- bind to non-io ancestor of GPU device(s)\n");
    printf("            ib{<id>|:<n>}    -- bind to non-io ancestor of IB device(s)\n");
    printf("            en|eth{<id>|:<n>} -- bind to non-io ancestor of Ethernet device(s)\n");
    printf("            hfi{<id>|:<n>}   -- bind to non-io ancestor of OPA device(s)\n");


    printf("\n\n");

    printf("-map-by: Order of bind mapping to use\n\n");
    printf("    Options (T: hwthread; C: core; S: socket; N: NUMA domain; B: motherboard):\n");
    printf("        Default: <same option as binding>\n");
    printf("\n");
    printf("        Architecture aware options:\n");
    printf("            machine          -- map by machine\n");
    printf("            numa{:<n>}       -- map by 'n' numa domains\n");
    printf("            socket{:<n>}     -- map by 'n' sockets\n");
    printf("            core{:<n>}       -- map by 'n' cores\n");
    printf("            hwthread{:<n>}   -- map by 'n' hardware threads\n");
    printf("            l1cache{:<n>}    -- map by 'n' L1 cache domains\n");
    printf("            l1dcache{:<n>}   -- map by 'n' L1 data cache domain\n");
    printf("            l1icache{:<n>}   -- map by 'n' L1 instruction cache domain\n");
    printf("            l1ucache{:<n>}   -- map by 'n' L1 unified cache domain\n");
    printf("            l2cache{:<n>}    -- map by 'n' L2 cache domains\n");
    printf("            l2dcache{:<n>}   -- map by 'n' L2 data cache domain\n");
    printf("            l2icache{:<n>}   -- map by 'n' L2 instruction cache domain\n");
    printf("            l2ucache{:<n>}   -- map by 'n' L2 unified cache domain\n");
    printf("            l3cache{:<n>}    -- map by 'n' L3 cache domain\n");
    printf("            l3dcache{:<n>}   -- map by 'n' L3 data cache domain\n");
    printf("            l3icache{:<n>}   -- map by 'n' L3 instruction cache domain\n");
    printf("            l3ucache{:<n>}   -- map by 'n' L3 unified cache domain\n");
    printf("            l4cache{:<n>}    -- map by 'n' L4 cache domain\n");
    printf("            l4dcache{:<n>}   -- map by 'n' L4 data cache domain\n");
    printf("            l4ucache{:<n>}   -- map by 'n' L4 unified cache domain\n");
    printf("            l5cache{:<n>}    -- map by 'n' L5 cache domain\n");
    printf("            l5dcache{:<n>}   -- map by 'n' L5 data cache domain\n");
    printf("            l5ucache{:<n>}   -- map by 'n' L5 unified cache domain\n");
    printf("            pci:<id>         -- map by non-io ancestor of PCI device\n");
    printf("                                (must match binding)\n");
    printf("            gpu{<id>|:<n>}   -- map by non-io ancestor of GPU device(s)\n");
    printf("                                (must match binding)\n");
    printf("            ib{<id>|:<n>}    -- map by non-io ancestor of IB device(s)\n");
    printf("                                (must match binding)\n");
    printf("            en|eth{<id>|:<n>} -- map by non-io ancestor of Ethernet device(s)\n");
    printf("                                (must match binding)\n");
    printf("            hfi{<id>|:<n>}   -- map by non-io ancestor of OPA device(s)\n");
    printf("                                (must match binding)\n");

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
}

static HYD_status bind_to_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.binding) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.mapping) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.membind) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.topolib) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.demux) {
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
    HYD_server_info.user_global.debug = 1;
    return HYD_SUCCESS;
}

static HYD_status vv_fn(char *arg, char ***argv)
{
    HYD_server_info.user_global.debug = 2;
    return HYD_SUCCESS;
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.print_all_exitcodes != -1) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.iface) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.nameserver) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.auto_cleanup != -1) {
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

static void order_nodes_help_fn(void)
{
    printf("\n");
    printf("-order-nodes ASCENDING: Sort the node list in ascending order\n");
    printf("-order-nodes DESCENDING: Sort the node list in descending order\n");
}

static HYD_status order_nodes_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file && HYD_ui_mpich_info.sort_order != NONE) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.localhost) {
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

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.usize) {
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

static void pmi_port_help_fn(void)
{
    printf("\n");
    printf("-pmi_port: Use the PMI_PORT environment instead of PMI_FD\n");
    printf("   PMI_PORT uses TCP sockets for PMI connections, instead of UNIX sockets\n");
    printf("   This model is safer when funneling through debuggers such as lldb\n");
}

static HYD_status pmi_port_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.usize) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.pmi_port != -1,
                        HYD_INTERNAL_ERROR, "PMI port option already set\n");

    HYD_server_info.user_global.pmi_port = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void skip_launch_node_help_fn(void)
{
    printf("\n");
    printf("-skip-launch-node: Do not run MPI processes on the launch node\n");
    printf("   The launch node is the node that runs mpiexec\n");
}

static HYD_status skip_launch_node_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file && HYD_server_info.user_global.skip_launch_node != -1) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.skip_launch_node != -1,
                        HYD_INTERNAL_ERROR, "Skip launch node already set\n");

    HYD_server_info.user_global.skip_launch_node = 1;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void pmi_args_help_fn(void)
{
    printf("\n");
    printf("-pmi-args port default_interface default_key pid:\n");
    printf("   Supply pmi-args when mpiexec is launched from singleton init.\n");
    printf("   Both port for mpiexec to connect back to the singleton process\n");
    printf("   and the pid of the singleton process are required. The interface\n");
    printf("   name and authentication key are ignored for now.\n");
}

static HYD_status pmi_args_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYD_server_info.singleton_port = atoi((*argv)[0]);
    HYD_server_info.singleton_pid = atoi((*argv)[3]);
    HYD_server_info.is_singleton = true;

    (*argv) += 4;

    return status;
}

static void gpus_per_proc_help_fn(void)
{
    printf("\n");
    printf("-gpus-per-proc: Number of GPUs assigned to each process\n");
    printf("   Sets the appropriate environment variables for CUDA\n");
    printf("   Users have to ensure that there are enough GPUs on each node\n\n");
    printf
        ("   AUTO: All GPUs if CUDA is shared compute mode and none in exclusive mode (default)\n");
    printf("   <value>: Numeric value >= 0\n\n");
}

static HYD_status gpus_per_proc_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file &&
        HYD_server_info.user_global.gpus_per_proc != HYD_GPUS_PER_PROC_UNSET) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status,
                        HYD_server_info.user_global.gpus_per_proc != HYD_GPUS_PER_PROC_UNSET,
                        HYD_INTERNAL_ERROR, "GPUs per proc already set\n");

    if (!strcmp(**argv, "AUTO")) {
        HYD_server_info.user_global.gpus_per_proc = HYD_GPUS_PER_PROC_AUTO;
    } else {
        HYD_server_info.user_global.gpus_per_proc = atoi(**argv);
        HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.gpus_per_proc < 0,
                            HYD_INTERNAL_ERROR, "invalid number of GPUs per proc\n");
    }

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void hybrid_hosts_help_fn(void)
{
    printf("\n");
    printf("-hybrid_hosts:\n");
    printf("   Assume hosts do not share PATH and working directories. For it\n");
    printf("   to work, hydra need be installed to PATH on individual hosts.\n");
    printf("   Working directory will not be set by default.\n");
}

static HYD_status hybrid_hosts_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYD_server_info.hybrid_hosts = true;

    return status;
}

static void gpu_subdevs_per_proc_help_fn(void)
{
    printf("\n");
    printf("-gpu-subdevs-per-proc: Number of GPU subdevices assigned to each process\n");
    printf("   Only support Intel ZE. Sets the appropriate environment variables for ZE\n");
    printf("   AUTO: All GPU subdevices (default)\n");
    printf("   <value>: Numeric value >= 0\n\n");
}

static HYD_status gpu_subdevs_per_proc_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (HYD_ui_mpich_info.reading_config_file &&
        HYD_server_info.user_global.gpu_subdevs_per_proc != HYD_GPUS_PER_PROC_UNSET) {
        /* global variable already set; ignore */
        goto fn_exit;
    }

    HYDU_ERR_CHKANDJUMP(status,
                        HYD_server_info.user_global.gpu_subdevs_per_proc != HYD_GPUS_PER_PROC_UNSET,
                        HYD_INTERNAL_ERROR, "GPUs per proc already set\n");

    if (!strcmp(**argv, "AUTO")) {
        HYD_server_info.user_global.gpu_subdevs_per_proc = HYD_GPUS_PER_PROC_AUTO;
    } else {
        HYD_server_info.user_global.gpu_subdevs_per_proc = atoi(**argv);
        HYDU_ERR_CHKANDJUMP(status, HYD_server_info.user_global.gpu_subdevs_per_proc < 0,
                            HYD_INTERNAL_ERROR, "invalid number of GPUs per proc\n");
    }

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

struct HYD_arg_match_table HYD_mpiexec_match_table[] = {
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
    {"rankmap", rankmap_fn, rankmap_help_fn},
    {"ppn", ppn_fn, ppn_help_fn},
    {"profile", profile_fn, profile_help_fn},
    {"output-from", output_from_fn, output_from_help_fn},
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
    {"debug", verbose_fn, verbose_help_fn},
    {"disable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"enable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"g", gpus_per_proc_fn, gpus_per_proc_help_fn},
    {"gpus-per-proc", gpus_per_proc_fn, gpus_per_proc_help_fn},
    {"gpu-subdevs-per-proc", gpu_subdevs_per_proc_fn, gpu_subdevs_per_proc_help_fn},
    {"hybrid-hosts", hybrid_hosts_fn, hybrid_hosts_help_fn},
    {"iface", iface_fn, iface_help_fn},
    {"info", info_fn, info_help_fn},
    {"localhost", localhost_fn, localhost_help_fn},
    {"nameserver", nameserver_fn, nameserver_help_fn},
    {"order-nodes", order_nodes_fn, order_nodes_help_fn},
    {"pmi-port", pmi_port_fn, pmi_port_help_fn},
    {"print-all-exitcodes", print_all_exitcodes_fn, print_all_exitcodes_help_fn},
    {"skip-launch-node", skip_launch_node_fn, skip_launch_node_help_fn},
    {"usize", usize_fn, usize_help_fn},
    {"v", verbose_fn, verbose_help_fn},
    {"vv", vv_fn, verbose_help_fn},
    {"verbose", verbose_fn, verbose_help_fn},
    {"version", info_fn, info_help_fn},

    /* Singleton init */
    {"pmi_args", pmi_args_fn, pmi_args_help_fn},

    {"\0", NULL, NULL}
};
