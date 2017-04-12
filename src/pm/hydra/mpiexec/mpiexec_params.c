/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "mpx.h"
#include "mpiexec.h"

static struct HYD_arg_match_table match_table[];
static struct mpiexec_pg *root_pg = NULL;

static HYD_status get_current_exec(struct mpiexec_pg *pg, struct HYD_exec **exec)
{
    HYD_status status = HYD_SUCCESS;

    HYD_ASSERT(pg, status);

    if (pg->exec_list == NULL) {
        status = HYD_exec_alloc(&pg->exec_list);
        HYD_ERR_POP(status, "unable to allocate exec\n");
    }

    *exec = pg->exec_list;
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

    printf("\n");
    printf("\n");

    printf("Local options (passed to individual executables):\n");

    printf("\n");
    printf("  Other local options:\n");
    printf("    -n/-np {value}                   number of processes\n");
    printf("    {exec_name} {args}               executable name and arguments\n");

    printf("\n");
    printf("\n");

    printf("Hydra specific options (treated as global):\n");

    printf("\n");
    printf("  Launch options:\n");
    printf("    -launcher                        launcher to use (%s)\n", HYDRA_AVAILABLE_BSTRAPS);
    printf("    -launcher-exec                   executable to use to launch processes\n");
    printf("    -enable-x/-disable-x             enable or disable X forwarding\n");

    printf("\n");
    printf("  Resource management kernel options:\n");
    printf("    -rmk                             resource management kernel to use (%s)\n",
           HYDRA_AVAILABLE_RMKS);

    printf("\n");
    printf("  Processor topology options:\n");
    printf("    -bind-to                         process binding\n");
    printf("    -map-by                          process mapping\n");
    printf("    -membind                         memory binding policy\n");

    printf("\n");
    printf("  Other Hydra options:\n");
    printf("    -verbose                         verbose mode\n");
    printf("    -info                            build information\n");
    printf("    -print-all-exitcodes             print exit codes of all processes\n");
    printf("    -ppn                             processes per node\n");
    printf("    -prepend-rank                    prepend rank to output\n");
    printf("    -prepend-pattern                 prepend pattern to output\n");
    printf("    -outfile-pattern                 direct stdout to file\n");
    printf("    -errfile-pattern                 direct stderr to file\n");
    printf("    -nameserver                      name server information (host:port format)\n");
    printf("    -disable-auto-cleanup            don't cleanup processes on error\n");
    printf("    -disable-hostname-propagation    let MPICH auto-detect the hostname\n");
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
    exit(0);

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void dump_env_notes(void)
{
    printf("Additional generic notes:\n");
    printf("  * There are two types of environments: primary and secondary\n");
    printf("      --> primary env is overwritten on each node, secondary is not\n");
    printf("      --> by default, secondary environment is the user's default environment\n");
    printf("                      primary environment is empty\n");
    printf("      --> providing -genvall forces the user env into primary\n");
    printf("      --> providing -genvnone removes the user env from secondary\n");
    printf("      --> providing -genvlist forces a subset of the user env into primary\n");
    printf("  * Multiple global env options cannot be used together\n");
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

    status = HYD_str_split(**argv, &str[0], &str[1], '=');
    HYD_ERR_POP(status, "string break returned error\n");
    (*argv)++;

    env_name = MPL_strdup(str[0]);
    if (str[1] == NULL) {       /* The environment is not of the form x=foo */
        if (**argv == NULL) {
            status = HYD_ERR_INTERNAL;
            goto fn_fail;
        }
        env_value = MPL_strdup(**argv);
        (*argv)++;
    }
    else {
        env_value = MPL_strdup(str[1]);
    }

    HYD_REALLOC(mpiexec_params.primary.env, char **,
                (mpiexec_params.primary.envcount + 1) * sizeof(char *), status);
    status = HYD_env_create(&env, env_name, env_value);
    HYD_ERR_POP(status, "error creating env\n");
    status = HYD_env_to_str(env, &mpiexec_params.primary.env[mpiexec_params.primary.envcount]);
    HYD_ERR_POP(status, "error converting env to string\n");
    mpiexec_params.primary.envcount++;

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
    int i;
    HYD_status status = HYD_SUCCESS;

    mpiexec_params.envprop = MPIEXEC_ENVPROP__LIST;

    /* parse through the list and find out how many variables we have */
    mpiexec_params.envlist_count = 1;
    for (i = 0; (**argv)[i]; i++)
        if ((**argv)[i] == ',')
            mpiexec_params.envlist_count++;

    HYD_MALLOC(mpiexec_params.envlist, char **, mpiexec_params.envlist_count, status);
    for (i = 0; i < mpiexec_params.envlist_count; i++) {
        if (i == 0)
            mpiexec_params.envlist[i] = MPL_strdup(strtok(**argv, ","));
        else
            mpiexec_params.envlist[i] = MPL_strdup(strtok(NULL, ","));
    }
    HYD_ASSERT(strtok(NULL, ","), status);

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

    mpiexec_params.envprop = MPIEXEC_ENVPROP__NONE;

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

    mpiexec_params.envprop = MPIEXEC_ENVPROP__ALL;

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
    char localhost[HYD_MAX_HOSTNAME_LEN] = { 0 };
    int max_global_node_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYD_ERR_CHKANDJUMP(status, mpiexec_params.global_node_count, HYD_ERR_INTERNAL,
                       "duplicate host file setting\n");

    if (strcmp(**argv, "HYDRA_USE_LOCALHOST")) {
        status =
            HYD_hostfile_parse(**argv, &mpiexec_params.global_node_count,
                               &mpiexec_params.global_node_list, HYD_hostfile_process_tokens);
        HYD_ERR_POP(status, "error parsing hostfile\n");
    }
    else {
        if (gethostname(localhost, HYD_MAX_HOSTNAME_LEN) < 0)
            HYD_ERR_SETANDJUMP(status, HYD_ERR_SOCK, "unable to get local hostname\n");

        status =
            HYD_node_list_append(localhost, 1, &mpiexec_params.global_node_list,
                                 &mpiexec_params.global_node_count, &max_global_node_count);
        HYD_ERR_POP(status, "unable to add to node list\n");
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
    int max_global_node_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYD_ERR_CHKANDJUMP(status, mpiexec_params.global_node_count, HYD_ERR_INTERNAL,
                       "duplicate host file or host list setting\n");

    hostlist[count] = strtok(**argv, ",");
    while ((count < HYD_NUM_TMP_STRINGS) && hostlist[count])
        hostlist[++count] = strtok(NULL, ",");

    if (count >= HYD_NUM_TMP_STRINGS)
        HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "too many hosts listed\n");

    for (count = 0; hostlist[count]; count++) {
        char *h, *procs = NULL;
        int np;

        h = strtok(hostlist[count], ":");
        procs = strtok(NULL, ":");
        np = procs ? atoi(procs) : 1;

        status =
            HYD_node_list_append(h, np, &mpiexec_params.global_node_list,
                                 &mpiexec_params.global_node_count, &max_global_node_count);
        HYD_ERR_POP(status, "unable to add to node list\n");
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

    status = HYD_arg_set_int(arg, &mpiexec_params.ppn, atoi(**argv));
    HYD_ERR_POP(status, "error setting ppn\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.prepend_pattern, "[%r] ");
    HYD_ERR_POP(status, "error setting prepend_rank field\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.prepend_pattern, **argv);
    HYD_ERR_POP(status, "error setting prepend pattern\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.outfile_pattern, **argv);
    HYD_ERR_POP(status, "error setting outfile pattern\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.errfile_pattern, **argv);
    HYD_ERR_POP(status, "error setting errfile pattern\n");

  fn_exit:
    (*argv)++;
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

    status = get_current_exec(root_pg, &exec);
    HYD_ERR_POP(status, "get_current_exec returned error\n");

    status = HYD_arg_set_int(arg, &exec->proc_count, atoi(**argv));
    HYD_ERR_POP(status, "error getting executable process count\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.launcher, **argv);
    HYD_ERR_POP(status, "error setting launcher\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.launcher_exec, **argv);
    HYD_ERR_POP(status, "error setting launcher exec\n");

  fn_exit:
    (*argv)++;
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

    status = HYD_arg_set_str(arg, &mpiexec_params.rmk, **argv);
    HYD_ERR_POP(status, "error setting rmk\n");

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

    status = HYD_arg_set_str(arg, &mpiexec_params.binding, **argv);
    HYD_ERR_POP(status, "error setting binding\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status map_by_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYD_arg_set_str(arg, &mpiexec_params.mapping, **argv);
    HYD_ERR_POP(status, "error setting mapping\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status membind_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYD_arg_set_str(arg, &mpiexec_params.membind, **argv);
    HYD_ERR_POP(status, "error setting mapping\n");

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

    status = HYD_arg_set_int(arg, &mpiexec_params.debug, 1);
    HYD_ERR_POP(status, "error setting debug\n");

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

    HYD_PRINT_NOPREFIX(stdout, "HYDRA build details:\n");
    HYD_PRINT_NOPREFIX(stdout, "    Version:                                 %s\n", HYDRA_VERSION);
    HYD_PRINT_NOPREFIX(stdout, "    Release Date:                            %s\n",
                       HYDRA_RELEASE_DATE);
    HYD_PRINT_NOPREFIX(stdout, "    CC:                              %s\n", HYDRA_CC);
    HYD_PRINT_NOPREFIX(stdout, "    CXX:                             %s\n", HYDRA_CXX);
    HYD_PRINT_NOPREFIX(stdout, "    F77:                             %s\n", HYDRA_F77);
    HYD_PRINT_NOPREFIX(stdout, "    F90:                             %s\n", HYDRA_F90);
    HYD_PRINT_NOPREFIX(stdout, "    Configure options:                       %s\n",
                       HYDRA_CONFIGURE_ARGS_CLEAN);
    HYD_PRINT_NOPREFIX(stdout, "    Process Manager:                         pmi\n");
    HYD_PRINT_NOPREFIX(stdout, "    Launchers available:                     %s\n",
                       HYDRA_AVAILABLE_BSTRAPS);
    HYD_PRINT_NOPREFIX(stdout, "    Resource management kernels available:   %s\n",
                       HYDRA_AVAILABLE_RMKS);

    exit(0);

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

    status = HYD_arg_set_int(arg, &mpiexec_params.print_all_exitcodes, 1);
    HYD_ERR_POP(status, "error setting print_all_exitcodes\n");

  fn_exit:
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

    status = HYD_arg_set_str(arg, &mpiexec_params.nameserver, **argv);
    HYD_ERR_POP(status, "error setting nameserver\n");

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

    status =
        HYD_arg_set_int(arg, &mpiexec_params.auto_cleanup, !strcmp(arg, "enable-auto-cleanup"));
    HYD_ERR_POP(status, "error setting auto cleanup\n");

  fn_exit:
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

    status = HYD_arg_set_str(arg, &mpiexec_params.localhost, **argv);
    HYD_ERR_POP(status, "error setting local hostname\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void usize_help_fn(void)
{
    printf("\n");
    printf("-usize: Universe size (SYSTEM, INFINITE, <value>)\n");
    printf("   SYSTEM: Number of cores passed to mpiexec through hostfile or resource manager\n");
    printf("   INFINITE: No limit\n");
    printf("   <value>: Numeric value >= 0\n\n");
}

static HYD_status usize_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYD_ERR_CHKANDJUMP(status, mpiexec_params.usize != MPIEXEC_USIZE__UNSET, HYD_ERR_INTERNAL,
                       "universe size already set\n");

    if (!strcmp(**argv, "SYSTEM"))
        mpiexec_params.usize = MPIEXEC_USIZE__SYSTEM;
    else if (!strcmp(**argv, "INFINITE"))
        mpiexec_params.usize = MPIEXEC_USIZE__INFINITE;
    else {
        mpiexec_params.usize = atoi(**argv);
        HYD_ERR_CHKANDJUMP(status, mpiexec_params.usize <= 0, HYD_ERR_INTERNAL,
                           "invalid universe size\n");
    }

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static void tree_width_help_fn(void)
{
    printf("\n");
    printf("-tree-width: Width of the bootstrap tree\n");
    printf("     the value 0 stands for infinite width\n\n");
}

static HYD_status tree_width_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYD_ERR_CHKANDJUMP(status, mpiexec_params.tree_width != -1, HYD_ERR_INTERNAL,
                       "tree width already set\n");

    mpiexec_params.tree_width = atoi(**argv);
    HYD_ERR_CHKANDJUMP(status, mpiexec_params.tree_width < 0, HYD_ERR_INTERNAL,
                       "invalid tree width\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status mpiexec_get_parameters(char **t_argv)
{
    char **argv = t_argv;
    char *progname = *argv;
    char *post, *loc, *tmp[HYD_NUM_TMP_STRINGS];
    struct HYD_exec *exec;
    char *tstr;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYD_FUNC_ENTER();

    status = mpiexec_alloc_pg(&root_pg, 0);
    HYD_ERR_POP(status, "error allocating PG\n");


    /***** COMMAND-LINE PARAMETERS ****/

    argv++;
    do {
        status = HYD_arg_parse_array(&argv, match_table);
        HYD_ERR_POP(status, "error parsing input array\n");

        if (!(*argv))
            break;

        /* Get the executable information */
        /* Read the executable till you hit the end of a ":" */
        do {
            status = get_current_exec(root_pg, &exec);
            HYD_ERR_POP(status, "get_current_exec returned error\n");

            if (!strcmp(*argv, ":")) {  /* Next executable */
                status = HYD_exec_alloc(&exec->next);
                HYD_ERR_POP(status, "allocate_exec returned error\n");
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



    /***** ENVIRONMENT-SET PARAMETERS ****/

    /* check if there is an environment set for the hostfile */
    if (mpiexec_params.global_node_count == 0) {
        tstr = NULL;
        MPL_env2str("HYDRA_HOST_FILE", (const char **) &tstr);
        if (tstr) {
            status =
                HYD_hostfile_parse(tstr, &mpiexec_params.global_node_count,
                                   &mpiexec_params.global_node_list, HYD_hostfile_process_tokens);
            HYD_ERR_POP(status, "error parsing hostfile\n");
        }
    }

    /* check if there is an environment set for the RMK */
    if (mpiexec_params.rmk == NULL) {
        if (MPL_env2str("HYDRA_RMK", (const char **) &mpiexec_params.rmk))
            mpiexec_params.rmk = MPL_strdup(mpiexec_params.rmk);
    }

    /* check if there's an environment set for PPN */
    if (mpiexec_params.ppn == -1) {
        tstr = NULL;
        if (MPL_env2str("HYDRA_PPN", (const char **) &tstr))
            mpiexec_params.ppn = atoi(tstr);
    }


    /***** AUTO-DETECT/COMPUTE PARAMETERS ****/

    /* Get the base path */
    /* Find the last '/' in the executable name */
    post = MPL_strdup(progname);
    loc = strrchr(post, '/');
    if (!loc) { /* If there is no path */
        mpiexec_params.base_path = NULL;
        status = HYD_find_in_path(progname, &mpiexec_params.base_path);
        HYD_ERR_POP(status, "error while searching for executable in the user path\n");
    }
    else {      /* There is a path */
        *(++loc) = 0;

        /* Check if its absolute or relative */
        if (post[0] != '/') {   /* relative */
            tmp[0] = HYD_getcwd();
            tmp[1] = MPL_strdup("/");
            tmp[2] = MPL_strdup(post);
            tmp[3] = NULL;
            status = HYD_str_alloc_and_join(tmp, &mpiexec_params.base_path);
            HYD_ERR_POP(status, "unable to join strings\n");
            HYD_str_free_list(tmp);
        }
        else {  /* absolute */
            mpiexec_params.base_path = MPL_strdup(post);
        }
    }
    MPL_free(post);


    /***** DEFAULT PARAMETERS ****/

    if (mpiexec_params.print_all_exitcodes == -1)
        mpiexec_params.print_all_exitcodes = 0;

    if (mpiexec_params.auto_cleanup == -1)
        mpiexec_params.auto_cleanup = 1;

    if (mpiexec_params.usize == MPIEXEC_USIZE__UNSET)
        mpiexec_params.usize = MPIEXEC_USIZE__INFINITE;

    if (mpiexec_params.tree_width == -1)
        mpiexec_params.tree_width = HYD_TREE_DEFAULT_WIDTH;


    /***** ERROR CHECKING ****/

    for (exec = root_pg->exec_list; exec; exec = exec->next) {
        if (exec->exec[0] == NULL)
            HYD_ERR_SETANDJUMP(status, HYD_ERR_INTERNAL, "no executable specified\n");
    }

  fn_exit:
    HYD_FUNC_EXIT();
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
    {"prepend-rank", prepend_rank_fn, prepend_rank_help_fn},
    {"l", prepend_rank_fn, prepend_rank_help_fn},
    {"prepend-pattern", prepend_pattern_fn, prepend_pattern_help_fn},
    {"outfile-pattern", outfile_pattern_fn, outfile_pattern_help_fn},
    {"errfile-pattern", errfile_pattern_fn, errfile_pattern_help_fn},
    {"outfile", outfile_pattern_fn, outfile_pattern_help_fn},
    {"errfile", errfile_pattern_fn, errfile_pattern_help_fn},

    /* Other local options */
    {"n", np_fn, np_help_fn},
    {"np", np_fn, np_help_fn},

    /* Hydra specific options */

    /* Launcher options */
    {"launcher", launcher_fn, launcher_help_fn},
    {"launcher-exec", launcher_exec_fn, launcher_exec_help_fn},

    /* Resource management kernel options */
    {"rmk", rmk_fn, rmk_help_fn},

    /* Topology options */
    {"binding", bind_to_fn, bind_to_help_fn},
    {"bind-to", bind_to_fn, bind_to_help_fn},
    {"map-by", map_by_fn, bind_to_help_fn},
    {"membind", membind_fn, bind_to_help_fn},

    /* Other hydra options */
    {"verbose", verbose_fn, verbose_help_fn},
    {"v", verbose_fn, verbose_help_fn},
    {"debug", verbose_fn, verbose_help_fn},
    {"info", info_fn, info_help_fn},
    {"version", info_fn, info_help_fn},
    {"print-all-exitcodes", print_all_exitcodes_fn, print_all_exitcodes_help_fn},
    {"nameserver", nameserver_fn, nameserver_help_fn},
    {"disable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"dac", auto_cleanup_fn, auto_cleanup_help_fn},
    {"enable-auto-cleanup", auto_cleanup_fn, auto_cleanup_help_fn},
    {"localhost", localhost_fn, localhost_help_fn},
    {"usize", usize_fn, usize_help_fn},
    {"tree-width", tree_width_fn, tree_width_help_fn},

    {"\0", NULL}
};
