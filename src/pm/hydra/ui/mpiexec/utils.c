/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "uiu.h"

#define HYDRA_MAX_PATH 4096

#define INCREMENT_ARGV(status)             \
    {                                      \
	if (!(++argv)) {                   \
	    (status) = HYD_INTERNAL_ERROR; \
	    goto fn_fail;                  \
	}                                  \
    }

#define IS_HELP(str) \
    ((!strcmp((str), "-h")) || (!strcmp((str), "-help")) || (!strcmp((str), "--help")))

static void show_version(void)
{
    printf("HYDRA build details:\n");
    printf("    Process Manager: pmi\n");
    printf("    Boot-strap servers available: %s\n", HYDRA_BSS_NAMES);
    printf("    Communication sub-systems available: none\n");
}

static void dump_env_notes(void)
{
    printf("Additional generic notes:\n");
    printf("  * Multiple global env options cannot be used together\n");
    printf("  * Multiple local env options cannot be used for the same executable\n");
    printf("  * Local env options override the global options for that executable\n");
    printf("\n");
}

HYD_Status HYD_UII_mpx_get_parameters(char **t_argv)
{
    int i, local_env_set;
    char **argv = t_argv, *tmp;
    char *env_name, *env_value, *str[4] = { NULL, NULL, NULL, NULL }, *progname = *argv;
    HYD_Env_t *env;
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_UIU_init_params();

    status = HYDU_list_global_env(&HYD_handle.global_env);
    HYDU_ERR_POP(status, "unable to get the global env list\n");

    if (IS_HELP(argv[1]))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "");

    while (++argv && *argv) {

        if (!strcmp(*argv, "-genvall")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate environment setting\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-genvall: All of the ENV is passed to each process (default)\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.prop = HYD_ENV_PROP_ALL;
            continue;
        }

        if (!strcmp(*argv, "-genvnone")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate environment setting\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-genvnone: None of the ENV is passed to each process\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.prop = HYD_ENV_PROP_NONE;
            continue;
        }

        if (!strcmp(*argv, "-genvlist")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate environment setting\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-genvlist: A list of ENV variables is passed to each process\n\n");
                printf("Notes:\n");
                printf("  * Usage: -genvlist env1,env2,env2\n");
                printf("  * Values of env1,env2,env3 are taken from the user ENV\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.prop = HYD_ENV_PROP_LIST;
            INCREMENT_ARGV(status);
            HYDU_comma_list_to_env_list(*argv, &HYD_handle.user_env);
            continue;
        }

        if (!strcmp(*argv, "-genv")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-genv: Specify an ENV-VALUE pair to be passed to each process\n\n");
                printf("Notes:\n");
                printf("  * Usage: -genv env1 val1\n");
                printf("  * Can be used multiple times for different variables\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            INCREMENT_ARGV(status);
            env_name = HYDU_strdup(*argv);
            INCREMENT_ARGV(status);
            env_value = HYDU_strdup(*argv);

            status = HYDU_env_create(&env, env_name, env_value);
            HYDU_ERR_POP(status, "unable to create env struct\n");

            HYDU_append_env_to_list(*env, &HYD_handle.user_env);
            continue;
        }

        if (!strcmp(*argv, "-envall")) {
            status = HYD_UIU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_ERR_CHKANDJUMP(status, exec_info->prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate environment setting\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-envall: All of the ENV is passed to this executable\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            exec_info->prop = HYD_ENV_PROP_ALL;
            continue;
        }

        if (!strcmp(*argv, "-envnone")) {
            status = HYD_UIU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_ERR_CHKANDJUMP(status, exec_info->prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate environment setting\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-envnone: None of the ENV is passed to this executable\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            exec_info->prop = HYD_ENV_PROP_NONE;
            continue;
        }

        if (!strcmp(*argv, "-envlist")) {
            status = HYD_UIU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_ERR_CHKANDJUMP(status, exec_info->prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate environment setting\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-envlist: A list of ENV variables is passed to this executable\n\n");
                printf("Notes:\n");
                printf("  * Usage: -envlist env1,env2,env2\n");
                printf("  * Values of env1,env2,env3 are taken from the user ENV\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            exec_info->prop = HYD_ENV_PROP_LIST;

            INCREMENT_ARGV(status);
            HYDU_comma_list_to_env_list(*argv, &exec_info->user_env);
            continue;
        }

        if (!strcmp(*argv, "-env")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-env: Specify an ENV-VALUE pair to be passed to this executable\n\n");
                printf("Notes:\n");
                printf("  * Usage: -env env1 val1\n");
                printf("  * Can be used multiple times for different variables\n\n");
                dump_env_notes();
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            INCREMENT_ARGV(status);
            env_name = HYDU_strdup(*argv);
            INCREMENT_ARGV(status);
            env_value = HYDU_strdup(*argv);

            status = HYDU_env_create(&env, env_name, env_value);
            HYDU_ERR_POP(status, "unable to create env struct\n");

            status = HYD_UIU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_append_env_to_list(*env, &exec_info->user_env);
            continue;
        }

        if (!strcmp(*argv, "-wdir")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-wdir: Working directory to use on the compute nodes\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            INCREMENT_ARGV(status);
            HYD_handle.wdir = HYDU_strdup(*argv);
            continue;
        }

        if (!strcmp(*argv, "-n") || !strcmp(*argv, "-np")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-n: Number of processes to launch\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            INCREMENT_ARGV(status);

            status = HYD_UIU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            if (exec_info->exec_proc_count != 0)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate process count\n");

            exec_info->exec_proc_count = atoi(*argv);
            continue;
        }

        if (!strcmp(*argv, "-f")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("-f: Host file containing compute node names\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            INCREMENT_ARGV(status);
            HYD_handle.host_file = HYDU_strdup(*argv);
            continue;
        }

        /* The below options are Hydra specific and allow for
         * "--foo=x" form of argument, instead of "--foo x" for
         * convenience. */
        for (i = 0; i < 4; i++)
            str[i] = NULL;

        status = HYDU_strsplit(*argv, &str[0], &str[1], '=');
        HYDU_ERR_POP(status, "string break returned error\n");

        if (!strcmp(str[0], "--version")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--version: Shows build information\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            show_version();
            HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
        }

        if (!strcmp(str[0], "--verbose")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--verbose: Prints additional debug information\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.debug = 1;
            continue;
        }

        if (!strcmp(str[0], "--print-rank-map")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--print-rank-map: Print what ranks are allocated to what nodes\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.print_rank_map = 1;
            continue;
        }

        if (!strcmp(str[0], "--print-all-exitcodes")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--print-all-exitcodes: Print termination codes of all processes\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.print_all_exitcodes = 1;
            continue;
        }

        if (!strcmp(str[0], "--enable-x") || !strcmp(str[0], "--disable-x")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.enablex != -1, HYD_INTERNAL_ERROR,
                                "duplicate --enable-x argument\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--enable-x: Enable X forwarding\n");
                printf("--disable-x: Disable X forwarding\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.enablex = !strcmp(str[0], "--enable-x");
            continue;
        }

        if (!strcmp(str[0], "--enable-pm-env") || !strcmp(str[0], "--disable-pm-env")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.pm_env != -1, HYD_INTERNAL_ERROR,
                                "duplicate --enable-pm-env argument\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--enable-pm-env: Enable ENV added by the process manager\n");
                printf("--disable-pm-env: Disable ENV added by the process manager\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.pm_env = !strcmp(str[0], "--enable-pm-env");
            continue;
        }

        if (!strcmp(str[0], "--boot-proxies")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.launch_mode != HYD_LAUNCH_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate launch mode\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--boot-proxies: Launch persistent proxies\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.launch_mode = HYD_LAUNCH_BOOT;
            continue;
        }

        if (!strcmp(str[0], "--boot-foreground-proxies")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.launch_mode != HYD_LAUNCH_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate launch mode\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--boot-foreground-proxies: ");
                printf("Launch persistent proxies in foreground\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.launch_mode = HYD_LAUNCH_BOOT_FOREGROUND;
            continue;
        }

        if (!strcmp(str[0], "--shutdown-proxies")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.launch_mode != HYD_LAUNCH_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate launch mode\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--shutdown-proxies: Shutdown persistent proxies\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            HYD_handle.launch_mode = HYD_LAUNCH_SHUTDOWN;
            continue;
        }

        if (!strcmp(str[0], "--use-persistent") || !strcmp(str[0], "--use-runtime")) {
            HYDU_ERR_CHKANDJUMP(status, HYD_handle.launch_mode != HYD_LAUNCH_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate launch mode\n");

            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--use-persistent: Use persistent proxies\n");
                printf("--use-runtime: Launch proxies at runtime\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!strcmp(str[0], "--use-persistent"))
                HYD_handle.launch_mode = HYD_LAUNCH_PERSISTENT;
            else
                HYD_handle.launch_mode = HYD_LAUNCH_RUNTIME;

            continue;
        }

        if (!strcmp(str[0], "--bootstrap")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--bootstrap: Bootstrap server to use\n\n");
                printf("Notes:\n");
                printf("  * Use the --version option to see what all are compiled in\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.bootstrap, HYD_INTERNAL_ERROR,
                                "duplicate --bootstrap option\n");
            HYD_handle.bootstrap = str[1];
            HYDU_FREE(str[0]);
            continue;
        }

        if (!strcmp(str[0], "--bootstrap-exec")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--bootstrap-exec: Bootstrap executable to use\n\n");
                printf("Notes:\n");
                printf("  * This is needed only if Hydra cannot automatically find it\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.bootstrap_exec, HYD_INTERNAL_ERROR,
                                "duplicate --bootstrap-exec option\n");
            HYD_handle.bootstrap_exec = str[1];
            HYDU_FREE(str[0]);
            continue;
        }

        if (!strcmp(str[0], "--rmk")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--rmk: Resource management kernel to use\n\n");
                printf("Notes:\n");
                printf("  * Use the --version option to see what all are compiled in\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.rmk, HYD_INTERNAL_ERROR,
                                "duplicate --rmk option\n");
            HYD_handle.rmk = str[1];
            HYDU_FREE(str[0]);
            continue;
        }

        if (!strcmp(str[0], "--css")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--css: Communication sub-system to use\n\n");
                printf("Notes:\n");
                printf("  * Use the --version option to see what all are compiled in\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.css, HYD_INTERNAL_ERROR,
                                "duplicate --css option\n");
            HYD_handle.css = str[1];
            HYDU_FREE(str[0]);
            continue;
        }

        if (!strcmp(str[0], "--binding")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--binding: Process-core binding to use\n\n");
                printf("Notes:\n");
                printf("  * Usage: --binding [type]; where [type] can be:\n");
                printf("        none: no binding\n");
                printf("        rr: round-robin as OS assigned processor IDs\n");
                printf("        buddy: order of least shared resources\n");
                printf("        pack: order of most shared resources\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.binding != HYD_BIND_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate binding\n");
            if (!strcmp(str[1], "none"))
                HYD_handle.binding = HYD_BIND_NONE;
            else if (!strcmp(str[1], "rr"))
                HYD_handle.binding = HYD_BIND_RR;
            else if (!strcmp(str[1], "buddy"))
                HYD_handle.binding = HYD_BIND_BUDDY;
            else if (!strcmp(str[1], "pack"))
                HYD_handle.binding = HYD_BIND_PACK;
            else {
                /* Check if the user wants to specify her own mapping */
                status = HYDU_strsplit(str[1], &str[2], &str[3], ':');
                HYDU_ERR_POP(status, "string break returned error\n");

                if (!strcmp(str[2], "user")) {
                    HYD_handle.binding = HYD_BIND_USER;
                    if (str[3])
                        HYD_handle.user_bind_map = str[3];
                    HYDU_FREE(str[2]);
                }
            }
            HYDU_FREE(str[0]);
            HYDU_FREE(str[1]);
            continue;
        }

        if (!strcmp(str[0], "--proxy-port")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--proxy-port: Port for persistent proxies to listen on\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.proxy_port != -1, HYD_INTERNAL_ERROR,
                                "duplicate --proxy-port option\n");
            HYD_handle.proxy_port = atoi(str[1]);
            HYDU_FREE(str[0]);
            HYDU_FREE(str[1]);
            continue;
        }

        if (!strcmp(str[0], "--ranks-per-proc")) {
            if (argv[1] && IS_HELP(argv[1])) {
                printf("\n");
                printf("--ranks-per-proc: MPI ranks to assign per launched process\n\n");
                HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
            }

            if (!str[1]) {
                /* Argument could be of the form "--foo x" */
                INCREMENT_ARGV(status);
                str[1] = HYDU_strdup(*argv);
            }

            HYDU_ERR_CHKANDJUMP(status, HYD_handle.ranks_per_proc != -1, HYD_INTERNAL_ERROR,
                                "duplicate --ranks-per-proc option\n");
            HYD_handle.ranks_per_proc = atoi(str[1]);
            HYDU_FREE(str[0]);
            HYDU_FREE(str[1]);
            continue;
        }

        /* Not a recognized argument of the form --foo=x OR --foo x */
        HYDU_FREE(str[0]);
        if (str[1])
            HYDU_FREE(str[1]);

        /* Prevent strings from being free'd twice */
        for (i = 0; i < 4; i++)
            str[i] = NULL;

        if (*argv[0] == '-')
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                                 "unrecognized argument %s\n", *argv);

        status = HYD_UIU_get_current_exec_info(&exec_info);
        HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

        /* End of Job launcher option handling */
        /* Read the executable till you hit the end of a ":" */
        do {
            if (!strcmp(*argv, ":")) {  /* Next executable */
                status = HYDU_alloc_exec_info(&exec_info->next);
                HYDU_ERR_POP(status, "allocate_exec_info returned error\n");
                break;
            }

            i = 0;
            while (exec_info->exec[i] != NULL)
                i++;
            exec_info->exec[i] = HYDU_strdup(*argv);
            exec_info->exec[i + 1] = NULL;
        } while (++argv && *argv);

        if (!(*argv))
            break;

        continue;
    }

    /* First set all the variables that do not depend on the launch mode */
    tmp = getenv("HYDRA_DEBUG");
    if (HYD_handle.debug == -1 && tmp)
        HYD_handle.debug = atoi(tmp) ? 1 : 0;
    if (HYD_handle.debug == -1)
        HYD_handle.debug = 0;

    tmp = getenv("HYDRA_PM_ENV");
    if (HYD_handle.pm_env == -1 && tmp)
        HYD_handle.pm_env = (atoi(getenv("HYDRA_PM_ENV")) != 0);
    if (HYD_handle.pm_env == -1)
        HYD_handle.pm_env = 1;      /* Default is to pass the PM environment */

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
    if (HYD_handle.host_file == NULL && tmp)
        HYD_handle.host_file = HYDU_strdup(tmp);

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

    /* Get the base path for the proxy */
    if (HYD_handle.wdir == NULL) {
        HYDU_MALLOC(HYD_handle.wdir, char *, HYDRA_MAX_PATH, status);
        if (getcwd(HYD_handle.wdir, HYDRA_MAX_PATH) == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "allocated space is too small for absolute path\n");
    }
    status = HYDU_get_base_path(progname, HYD_handle.wdir, &HYD_handle.base_path);
    HYDU_ERR_POP(status, "unable to get base path\n");

    /* Proxy setup or teardown */
    if ((HYD_handle.launch_mode == HYD_LAUNCH_BOOT) ||
        (HYD_handle.launch_mode == HYD_LAUNCH_BOOT_FOREGROUND) ||
        (HYD_handle.launch_mode == HYD_LAUNCH_SHUTDOWN)) {

        /* NULL out variables we don't care about */
        HYDU_ERR_CHKANDJUMP(status, HYD_handle.prop != HYD_ENV_PROP_UNSET, HYD_INTERNAL_ERROR,
                            "env setting not required for booting/shutting proxies\n");
        HYD_handle.prop = HYD_ENV_PROP_NONE;

        HYDU_ERR_CHKANDJUMP(status, HYD_handle.binding != HYD_BIND_UNSET, HYD_INTERNAL_ERROR,
                            "binding not allowed while booting/shutting proxies\n");
        HYD_handle.binding = HYD_BIND_UNSET;

        HYDU_ERR_CHKANDJUMP(status, HYD_handle.exec_info_list, HYD_INTERNAL_ERROR,
                            "executables should not be specified while booting/shutting proxies\n");
    }
    else {      /* Application launch */
        if (HYD_handle.exec_info_list == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        /* Check environment for setting binding */
        tmp = getenv("HYDRA_BINDING");
        if (HYD_handle.binding == HYD_BIND_UNSET && tmp)
            HYD_handle.binding = !strcmp(tmp, "none") ? HYD_BIND_NONE :
                !strcmp(tmp, "rr") ? HYD_BIND_RR :
                !strcmp(tmp, "buddy") ? HYD_BIND_BUDDY :
                !strcmp(tmp, "pack") ? HYD_BIND_PACK : HYD_BIND_USER;
        if (HYD_handle.binding == HYD_BIND_UNSET)
            HYD_handle.binding = HYD_BIND_NONE;

        /* Check environment for setting the global environment */
        tmp = getenv("HYDRA_ENV");
        if (HYD_handle.prop == HYD_ENV_PROP_UNSET && tmp)
            HYD_handle.prop = !strcmp(tmp, "all") ? HYD_ENV_PROP_ALL : HYD_ENV_PROP_NONE;

        /* Make sure local executable is set */
        local_env_set = 0;
        for (exec_info = HYD_handle.exec_info_list; exec_info; exec_info = exec_info->next) {
            if (exec_info->exec[0] == NULL)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

            if (exec_info->exec_proc_count == 0)
                exec_info->exec_proc_count = 1;

            if (exec_info->prop != HYD_ENV_PROP_UNSET)
                local_env_set = 1;
        }

        /* If no global or local environment is set, use the default */
        if ((HYD_handle.prop == HYD_ENV_PROP_UNSET) && (local_env_set == 0))
            HYD_handle.prop = HYD_ENV_PROP_ALL;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    for (i = 0; i < 4; i++)
        if (str[i])
            HYDU_FREE(str[i]);
    goto fn_exit;
}
