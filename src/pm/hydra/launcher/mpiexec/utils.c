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

#define CHECK_NEXT_ARG_VALID(status)            \
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
        HYDU_ERR_POP(status, "unable to allocate proc_params\n");
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


HYD_Status HYD_LCHI_get_parameters(int t_argc, char **t_argv)
{
    int argc = t_argc, i;
    char **argv = t_argv;
    char *env_name, *env_value, *str1, *str2, *progname = *argv;
    HYD_Env_t *env;
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_LCHU_init_params();

    status = HYDU_list_global_env(&handle.global_env);
    HYDU_ERR_POP(status, "unable to get the global env list\n");

    while (--argc && ++argv) {

        status = HYDU_strsplit(*argv, &str1, &str2, '=');
        HYDU_ERR_POP(status, "string break returned error\n");

        if (!strcmp(str1, "-h") || !strcmp(str1, "--help") || !strcmp(str1, "-help"))
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "");

        if (!strcmp(str1, "--verbose")) {
            if (handle.debug != -1)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate debug level\n");
            handle.debug = 1;
            continue;
        }

        if (!strcmp(str1, "--version")) {
            show_version();
            HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
        }

        if (!strcmp(str1, "--bootstrap")) {
            if (!str2) {
                CHECK_NEXT_ARG_VALID(status);
                str2 = *argv;
            }
            if (handle.bootstrap != NULL)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate bootstrap\n");
            handle.bootstrap = MPIU_Strdup(str2);
            continue;
        }

        if (!strcmp(str1, "--enable-x") || !strcmp(str1, "--disable-x")) {
            if (handle.enablex != -1)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate enable-x\n");
            handle.enablex = !strcmp(str1, "--enable-x");
            continue;
        }

        if (!strcmp(str1, "--proxy-port")) {
            if (!str2) {
                CHECK_NEXT_ARG_VALID(status);
                str2 = *argv;
            }
            if (handle.proxy_port != -1)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate proxy port\n");
            handle.proxy_port = atoi(str2);
            continue;
        }

        if (!strcmp(str1, "-genvall") || !strcmp(str1, "-genvnone") ||
            !strcmp(str1, "-genvlist")) {
            if (handle.prop != HYD_ENV_PROP_UNSET)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate prop setting\n");

            if (!strcmp(str1, "-genvall"))
                handle.prop = HYD_ENV_PROP_ALL;
            else if (!strcmp(str1, "-genvnone"))
                handle.prop = HYD_ENV_PROP_NONE;
            else if (!strcmp(str1, "-genvlist")) {
                handle.prop = HYD_ENV_PROP_LIST;

                if (!str2) {
                    CHECK_NEXT_ARG_VALID(status);
                    str2 = *argv;
                }

                env_name = strtok(str2, ",");
                do {
                    status = HYDU_env_create(&env, env_name, NULL);
                    HYDU_ERR_POP(status, "unable to create env struct\n");

                    status = HYDU_append_env_to_list(*env, &handle.user_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                } while ((env_name = strtok(NULL, ",")));
            }
            continue;
        }

        if (!strcmp(str1, "-envall") || !strcmp(str1, "-envnone") ||
            !strcmp(str1, "-envlist")) {
            status = get_current_proc_params(&proc_params);
            HYDU_ERR_POP(status, "get_current_proc_params returned error\n");

            if (proc_params->prop != HYD_ENV_PROP_UNSET)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate prop setting\n");

            if (!strcmp(str1, "-envall"))
                proc_params->prop = HYD_ENV_PROP_ALL;
            else if (!strcmp(str1, "-envnone"))
                proc_params->prop = HYD_ENV_PROP_NONE;
            else if (!strcmp(str1, "-envlist")) {
                proc_params->prop = HYD_ENV_PROP_LIST;

                if (!str2) {
                    CHECK_NEXT_ARG_VALID(status);
                    str2 = *argv;
                }

                do {
                    env_name = strtok(str2, ",");
                    if (env_name == NULL)
                        break;

                    status = HYDU_env_create(&env, env_name, NULL);
                    HYDU_ERR_POP(status, "unable to create env struct\n");

                    status = HYDU_append_env_to_list(*env, &proc_params->prop_env);
                    HYDU_ERR_POP(status, "unable to add env to list\n");
                } while (env_name);
            }
            continue;
        }

        /* Add additional environment variables */
        if (!strcmp(str1, "-genv") || !strcmp(str1, "-env")) {
            if (!str2) {
                CHECK_NEXT_ARG_VALID(status);
                str2 = *argv;
            }
            env_name = MPIU_Strdup(str2);

            CHECK_NEXT_ARG_VALID(status);
            env_value = MPIU_Strdup(*argv);

            status = HYDU_env_create(&env, env_name, env_value);
            HYDU_ERR_POP(status, "unable to create env struct\n");

            if (!strcmp(str1, "-genv"))
                HYDU_append_env_to_list(*env, &handle.user_env);
            else {
                status = get_current_proc_params(&proc_params);
                HYDU_ERR_POP(status, "get_current_proc_params returned error\n");

                HYDU_append_env_to_list(*env, &proc_params->user_env);
            }

            HYDU_FREE(env);
            continue;
        }

        if (!strcmp(str1, "-wdir")) {
            if (!str2) {
                CHECK_NEXT_ARG_VALID(status);
                str2 = *argv;
            }
            handle.wdir = MPIU_Strdup(str2);
            continue;
        }

        if (!strcmp(str1, "-n") || !strcmp(str1, "-np")) {
            if (!str2) {
                CHECK_NEXT_ARG_VALID(status);
                str2 = *argv;
            }

            status = get_current_proc_params(&proc_params);
            HYDU_ERR_POP(status, "get_current_proc_params returned error\n");

            if (proc_params->exec_proc_count != 0)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate proc count\n");

            proc_params->exec_proc_count = atoi(str2);
            continue;
        }

        if (!strcmp(str1, "-f")) {
            if (!str2) {
                CHECK_NEXT_ARG_VALID(status);
                str2 = *argv;
            }
            handle.host_file = MPIU_Strdup(str2);
            continue;
        }

        if (*argv[0] == '-')
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized argument\n");

        status = get_current_proc_params(&proc_params);
        HYDU_ERR_POP(status, "get_current_proc_params returned error\n");

        /* Read the executable till you hit the end of a ":" */
        do {
            if (!strcmp(*argv, ":")) {  /* Next executable */
                status = allocate_proc_params(&proc_params->next);
                HYDU_ERR_POP(status, "allocate_proc_params returned error\n");
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

    if (handle.debug == -1)
        handle.debug = 0;

    if (handle.proc_params == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no local options set\n");

    if (handle.wdir == NULL) {
        HYDU_MALLOC(handle.wdir, char *, HYDRA_MAX_PATH, status);
        if (getcwd(handle.wdir, HYDRA_MAX_PATH) == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "allocated space is too small for absolute path\n");
    }

    /*
     * We use the following priority order to specify the bootstrap server:
     *    1. Specified to mpiexec using --bootstrap
     *    2. Specified through the environment HYDRA_BOOTSTRAP
     *    3. If nothing is given, we use the default bootstrap server
     */
    if (handle.bootstrap == NULL && getenv("HYDRA_BOOTSTRAP"))
        handle.bootstrap = MPIU_Strdup(getenv("HYDRA_BOOTSTRAP"));
    if (handle.bootstrap == NULL)
        handle.bootstrap = MPIU_Strdup(HYDRA_DEFAULT_BSS);

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

    status = HYDU_get_base_path(progname, handle.wdir, &handle.base_path);
    HYDU_ERR_POP(status, "unable to get base path\n");

    proc_params = handle.proc_params;
    while (proc_params) {
        if (proc_params->exec[0] == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        if (proc_params->exec_proc_count == 0)
            proc_params->exec_proc_count = 1;

        if (handle.prop == HYD_ENV_PROP_UNSET && proc_params->prop == HYD_ENV_PROP_UNSET)
            proc_params->prop = HYD_ENV_PROP_ALL;

        proc_params = proc_params->next;
    }

    if (handle.proxy_port == -1)
        handle.proxy_port = HYD_DEFAULT_PROXY_PORT;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


void HYD_LCHI_print_parameters(void)
{
    HYD_Env_t *env;
    int i, j;
    struct HYD_Proc_params *proc_params;
    struct HYD_Partition_list *partition;

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

    return;
}
