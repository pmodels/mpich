/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "lchu.h"
#include "mpi.h"

#define HYDRA_MAX_PATH 4096

HYD_Handle handle;

#define CHECK_LOCAL_PARAM_START(start, status) \
    { \
	if ((start)) {                          \
	    (status) = HYD_INTERNAL_ERROR;	\
	    goto fn_fail; \
	} \
    }

#define CHECK_NEXT_ARG_VALID(status) \
    { \
	--argc; ++argv; \
	if (!argc || !argv) { \
	    (status) = HYD_INTERNAL_ERROR;	\
	    goto fn_fail; \
	} \
    }

static HYD_Status allocate_proc_params(struct HYD_Proc_params **params)
{
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(proc_params, struct HYD_Proc_params *, sizeof(struct HYD_Proc_params), status);

    proc_params->exec_proc_count = 0;
    proc_params->partition = NULL;

    proc_params->exec[0] = NULL;
    proc_params->user_env = NULL;
    proc_params->prop = HYD_ENV_PROP_UNSET;
    proc_params->prop_env = NULL;
    proc_params->stdout_cb = NULL;
    proc_params->stderr_cb = NULL;
    proc_params->next = NULL;

    *params = proc_params;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status get_current_proc_params(struct HYD_Proc_params **params)
{
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.proc_params == NULL) {
        status = allocate_proc_params(&handle.proc_params);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("unable to allocate proc_params\n");
            goto fn_fail;
        }
    }

    proc_params = handle.proc_params;
    while (proc_params->next)
        proc_params = proc_params->next;

    *params = proc_params;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static void show_version(void)
{
    printf("MPICH2 Version: %s\n", MPICH2_VERSION);
    printf("Process Manager: HYDRA\n");
    printf("Boot-strap servers available: %s\n", HYDRA_BSS_NAMES);
}


HYD_Status HYD_LCHI_Get_parameters(int t_argc, char **t_argv)
{
    int argc = t_argc, i;
    char **argv = t_argv;
    int local_params_started;
    char *env_name, *env_value;
    HYD_Env_t *env;
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_LCHU_Init_params();

    status = HYDU_Get_base_path(argv[0], &handle.base_path);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to get base path\n");
        goto fn_fail;
    }

    status = HYDU_Env_global_list(&handle.global_env);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to get the global env list\n");
        goto fn_fail;
    }

    local_params_started = 0;
    while (--argc && ++argv) {

        /* Help options */
        if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help") || !strcmp(*argv, "-help")) {
            /* Just return from this function; the main code will show the usage */
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }

        /* Check what debug level is requested */
        if (!strcmp(*argv, "--verbose")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* Debug level already set */
            if (handle.debug != -1) {
                HYDU_Error_printf
                    ("Duplicate debug level setting; previously set to %d\n", handle.debug);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }
            handle.debug = 1;

            continue;
        }

        /* Version information */
        if (!strcmp(*argv, "--version")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* Just show the version information and continue. This
             * option can be used in conjunction with other
             * options. */
            show_version();
            status = HYD_GRACEFUL_ABORT;
            goto fn_fail;

            continue;
        }

        /* Check if X forwarding is explicitly set */
        if (!strcmp(*argv, "--enable-x") || !strcmp(*argv, "--disable-x")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* X forwarding already enabled or disabled */
            if (handle.enablex != -1) {
                HYDU_Error_printf
                    ("Duplicate enable-x setting; previously set to %d\n", handle.enablex);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            handle.enablex = !strcmp(*argv, "--enable-x");
            continue;
        }

        /* Check if the proxy port is set */
        if (!strcmp(*argv, "--proxy-port")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);
            CHECK_NEXT_ARG_VALID(status);

            if (handle.proxy_port != -1) {
                HYDU_Error_printf("Duplicate proxy port setting; previously set to %d\n",
                                  handle.proxy_port);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            handle.proxy_port = atoi(*argv);
            continue;
        }

        /* Check what all environment variables need to be propagated */
        if (!strcmp(*argv, "-genvall") || !strcmp(*argv, "-genvnone") ||
            !strcmp(*argv, "-genvlist")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* propagation already set */
            if (handle.prop != HYD_ENV_PROP_UNSET) {
                HYDU_Error_printf
                    ("Duplicate propagation setting; previously set to %d\n", handle.prop);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            if (!strcmp(*argv, "-genvall")) {
                handle.prop = HYD_ENV_PROP_ALL;
            }
            else if (!strcmp(*argv, "-genvnone")) {
                handle.prop = HYD_ENV_PROP_NONE;
            }
            else if (!strcmp(*argv, "-genvlist")) {
                handle.prop = HYD_ENV_PROP_LIST;
                CHECK_NEXT_ARG_VALID(status);
                env_name = strtok(*argv, ",");
                do {
                    status = HYDU_Env_create(&env, env_name, NULL);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to create env struct\n");
                        goto fn_fail;
                    }

                    status = HYDU_Env_add_to_list(&handle.user_env, *env);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to add env to list\n");
                        goto fn_fail;
                    }
                } while ((env_name = strtok(NULL, ",")));
            }
            continue;
        }

        /* Check what all environment variables need to be propagated */
        if (!strcmp(*argv, "-envall") || !strcmp(*argv, "-envnone") ||
            !strcmp(*argv, "-envlist")) {
            local_params_started = 1;

            status = get_current_proc_params(&proc_params);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("get_current_proc_params returned error\n");
                goto fn_fail;
            }

            /* propagation already set */
            if (proc_params->prop != HYD_ENV_PROP_UNSET) {
                HYDU_Error_printf
                    ("Duplicate propagation setting; previously set to %d\n",
                     proc_params->prop);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            if (!strcmp(*argv, "-envall")) {
                proc_params->prop = HYD_ENV_PROP_ALL;
            }
            else if (!strcmp(*argv, "-envnone")) {
                proc_params->prop = HYD_ENV_PROP_NONE;
            }
            else if (!strcmp(*argv, "-envlist")) {
                proc_params->prop = HYD_ENV_PROP_LIST;
                CHECK_NEXT_ARG_VALID(status);
                do {
                    env_name = strtok(*argv, ",");
                    if (env_name == NULL)
                        break;

                    status = HYDU_Env_create(&env, env_name, NULL);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to create env struct\n");
                        goto fn_fail;
                    }

                    status = HYDU_Env_add_to_list(&proc_params->prop_env, *env);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to add env to list\n");
                        goto fn_fail;
                    }
                } while (env_name);
            }
            continue;
        }

        /* Add additional environment variables */
        if (!strcmp(*argv, "-genv") || !strcmp(*argv, "-env")) {
            if (!strcmp(*argv, "-genv"))
                CHECK_LOCAL_PARAM_START(local_params_started, status);

            CHECK_NEXT_ARG_VALID(status);
            env_name = MPIU_Strdup(*argv);

            CHECK_NEXT_ARG_VALID(status);
            env_value = MPIU_Strdup(*argv);

            status = HYDU_Env_create(&env, env_name, env_value);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("unable to create env struct\n");
                goto fn_fail;
            }

            if (!strcmp(*argv, "-genv"))
                HYDU_Env_add_to_list(&handle.user_env, *env);
            else {
                local_params_started = 1;

                status = get_current_proc_params(&proc_params);
                if (status != HYD_SUCCESS) {
                    HYDU_Error_printf("get_current_proc_params returned error\n");
                    goto fn_fail;
                }

                HYDU_Env_add_to_list(&proc_params->user_env, *env);
            }

            HYDU_FREE(env);
            continue;
        }

        if (!strcmp(*argv, "-wdir")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);
            CHECK_NEXT_ARG_VALID(status);
            handle.wdir = MPIU_Strdup(*argv);
            continue;
        }

        if (!strcmp(*argv, "-n") || !strcmp(*argv, "-np")) {
            local_params_started = 1;
            CHECK_NEXT_ARG_VALID(status);

            status = get_current_proc_params(&proc_params);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("get_current_proc_params returned error\n");
                goto fn_fail;
            }

            /* Num_procs already set */
            if (proc_params->exec_proc_count != 0) {
                HYDU_Error_printf
                    ("Duplicate setting for number of processes; previously set to %d\n",
                     proc_params->exec_proc_count);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            proc_params->exec_proc_count = atoi(*argv);

            continue;
        }

        if (!strcmp(*argv, "-f")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);
            CHECK_NEXT_ARG_VALID(status);
            handle.host_file = MPIU_Strdup(*argv);
            continue;
        }

        if (*argv[0] == '-') {
            HYDU_Error_printf("unrecognized argument\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }

        /* This is to catch everything that fell through */
        local_params_started = 1;

        status = get_current_proc_params(&proc_params);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("get_current_proc_params returned error\n");
            goto fn_fail;
        }

        /* Read the executable till you hit the end of a ":" */
        do {
            if (!strcmp(*argv, ":")) {  /* Next executable */
                status = allocate_proc_params(&proc_params->next);
                if (status != HYD_SUCCESS) {
                    HYDU_Error_printf("allocate_proc_params returned error\n");
                    goto fn_fail;
                }
                break;
            }

            i = 0;
            while (proc_params->exec[i] != NULL)
                i++;
            proc_params->exec[i] = MPIU_Strdup(*argv);
            proc_params->exec[i + 1] = NULL;
        } while (--argc && ++argv);

        if (!argc || !argv)
            break;

        continue;
    }

    /* Check on what's set and what's not */
    if (handle.debug == -1)
        handle.debug = 0;

    /* We need at least one local exec */
    if (handle.proc_params == NULL) {
        HYDU_Error_printf("No local options set\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    /* If wdir is not set, use the current one */
    if (handle.wdir == NULL) {
        HYDU_MALLOC(handle.wdir, char *, HYDRA_MAX_PATH, status);
        if (getcwd(handle.wdir, HYDRA_MAX_PATH) == NULL) {
            HYDU_Error_printf("allocated space is too small for absolute path\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }
    }

    /*
     * We use the following priority order to specify the host file:
     *    1. Specified to mpiexec using -f
     *    2. Specified through the environment HYDRA_HOST_FILE
     *    3. If nothing is given, we use the local host
     */
    if (handle.host_file == NULL && getenv("HYDRA_HOST_FILE"))
        handle.host_file = MPIU_Strdup(getenv("HYDRA_HOST_FILE"));
    if (handle.host_file == NULL)
        handle.host_file = MPIU_Strdup("HYDRA_USE_LOCALHOST");

    proc_params = handle.proc_params;
    while (proc_params) {
        if (proc_params->exec[0] == NULL) {
            HYDU_Error_printf("no executable specified\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }

        if (proc_params->exec_proc_count == 0)
            proc_params->exec_proc_count = 1;

        if (handle.prop == HYD_ENV_PROP_UNSET && proc_params->prop == HYD_ENV_PROP_UNSET) {
            /* By default we pass the entire environment */
            proc_params->prop = HYD_ENV_PROP_ALL;
        }

        proc_params = proc_params->next;
    }

    /* If the proxy port is not set, set it to the default value */
    if (handle.proxy_port == -1) {
        handle.proxy_port = HYD_DEFAULT_PROXY_PORT;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_LCHI_Print_parameters(void)
{
    HYD_Env_t *env;
    int i, j;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_Debug("\n");
    HYDU_Debug("=================================================");
    HYDU_Debug("=================================================");
    HYDU_Debug("\n");
    HYDU_Debug("mpiexec options:\n");
    HYDU_Debug("----------------\n");
    HYDU_Debug("  Base path: %s\n", handle.base_path);
    HYDU_Debug("  Proxy port: %d\n", handle.proxy_port);
    HYDU_Debug("  Bootstrap server: %s\n", handle.bootstrap);
    HYDU_Debug("  Debug level: %d\n", handle.debug);
    HYDU_Debug("  Enable X: %d\n", handle.enablex);
    HYDU_Debug("  Working dir: %s\n", handle.wdir);
    HYDU_Debug("  Host file: %s\n", handle.host_file);

    HYDU_Debug("\n");
    HYDU_Debug("  Global environment:\n");
    HYDU_Debug("  -------------------\n");
    for (env = handle.global_env; env; env = env->next)
        HYDU_Debug("    %s=%s\n", env->env_name, env->env_value);

    if (handle.system_env) {
        HYDU_Debug("\n");
        HYDU_Debug("  Hydra internal environment:\n");
        HYDU_Debug("  ---------------------------\n");
        for (env = handle.system_env; env; env = env->next)
            HYDU_Debug("    %s=%s\n", env->env_name, env->env_value);
    }

    if (handle.user_env) {
        HYDU_Debug("\n");
        HYDU_Debug("  User set environment:\n");
        HYDU_Debug("  ---------------------\n");
        for (env = handle.user_env; env; env = env->next)
            HYDU_Debug("    %s=%s\n", env->env_name, env->env_value);
    }

    HYDU_Debug("\n\n");

    HYDU_Debug("    Process parameters:\n");
    HYDU_Debug("    *******************\n");
    i = 1;
    for (proc_params = handle.proc_params; proc_params; proc_params = proc_params->next) {
        HYDU_Debug("      Executable ID: %2d\n", i++);
        HYDU_Debug("      -----------------\n");
        HYDU_Debug("        Process count: %d\n", proc_params->exec_proc_count);
        HYDU_Debug("        Executable: ");
        for (j = 0; proc_params->exec[j]; j++)
            HYDU_Debug("%s ", proc_params->exec[j]);
        HYDU_Debug("\n");

        if (proc_params->user_env) {
            HYDU_Debug("\n");
            HYDU_Debug("        User set environment:\n");
            HYDU_Debug("        .....................\n");
            for (env = proc_params->user_env; env; env = env->next)
                HYDU_Debug("          %s=%s\n", env->env_name, env->env_value);
        }

        j = 0;
        for (partition = proc_params->partition; partition; partition = partition->next) {
            HYDU_Debug("\n");
            HYDU_Debug("        Partition ID: %2d\n", j++);
            HYDU_Debug("        ----------------\n");
            HYDU_Debug("          Partition name: %s\n", partition->name);
            HYDU_Debug("          Partition process count: %d\n", partition->proc_count);
            HYDU_Debug("\n");
        }
    }

    HYDU_Debug("\n");
    HYDU_Debug("=================================================");
    HYDU_Debug("=================================================");
    HYDU_Debug("\n\n");

    HYDU_FUNC_EXIT();
    return status;
}
