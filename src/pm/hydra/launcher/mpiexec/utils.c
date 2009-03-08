/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_dbg.h"
#include "hydra_mem.h"
#include "hydra_env.h"
#include "mpiexec.h"
#include "lchu.h"

#define HYDRA_MAX_PATH 4096

HYD_Handle handle;
int HYDU_Dbg_depth;

#define CHECK_LOCAL_PARAM_START(start, status) \
    { \
	if ((start)) {			 \
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


HYD_Status HYD_LCHI_Get_parameters(int t_argc, char **t_argv)
{
    int argc = t_argc, i;
    char **argv = t_argv;
    int local_params_started;
    char *arg;
    char *env_name, *env_value;
    HYD_Env_t *env;
    struct HYD_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    handle.debug = -1;
    handle.enablex = -1;
    handle.wdir = NULL;
    handle.host_file = NULL;

    status = HYDU_Env_global_list(&handle.global_env);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to get the global env list\n");
        goto fn_fail;
    }
    handle.system_env = NULL;
    handle.user_env = NULL;
    handle.prop = HYD_ENV_PROP_UNSET;
    handle.prop_env = NULL;

    handle.proc_params = NULL;

    local_params_started = 0;
    while (--argc && ++argv) {

        /* Help options */
        if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help") || !strcmp(*argv, "-help")) {
            /* Just return from this function; the main code will show the usage */
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }

        /* Check what debug level is requested */
        if (!strcmp(*argv, "-v") || !strcmp(*argv, "-vv") || !strcmp(*argv, "-vvv")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* Debug level already set */
            if (handle.debug != -1) {
                HYDU_Error_printf("Duplicate debug level setting; previously set to %d\n", handle.debug);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            if (!strcmp(*argv, "-v"))
                handle.debug = 1;
            else if (!strcmp(*argv, "-vv"))
                handle.debug = 2;
            else if (!strcmp(*argv, "-vvv"))
                handle.debug = 3;

            continue;
        }

        /* Check if X forwarding is explicitly set */
        if (!strcmp(*argv, "--enable-x") || !strcmp(*argv, "--disable-x")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* X forwarding already enabled or disabled */
            if (handle.enablex != -1) {
                HYDU_Error_printf("Duplicate enable-x setting; previously set to %d\n", handle.enablex);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            handle.enablex = !strcmp(*argv, "--enable-x");
            continue;
        }

        /* Check what all environment variables need to be propagated */
        if (!strcmp(*argv, "-genvall") || !strcmp(*argv, "-genvnone") || !strcmp(*argv, "-genvlist")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* propagation already set */
            if (handle.prop != HYD_ENV_PROP_UNSET) {
                HYDU_Error_printf("Duplicate propagation setting; previously set to %d\n", handle.prop);
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
                do {
                    env_name = strtok(*argv, ",");
                    if (env_name == NULL)
                        break;

                    status = HYDU_Env_create(&env, env_name, NULL, HYD_ENV_STATIC, 0);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to create env struct\n");
                        goto fn_fail;
                    }

                    status = HYDU_Env_add_to_list(&handle.prop_env, *env);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to add env to list\n");
                        goto fn_fail;
                    }
                } while (env_name);
            }
            continue;
        }

        /* Check what all environment variables need to be propagated */
        if (!strcmp(*argv, "-envall") || !strcmp(*argv, "-envnone") || !strcmp(*argv, "-envlist")) {
            local_params_started = 1;

            status = get_current_proc_params(&proc_params);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("get_current_proc_params returned error\n");
                goto fn_fail;
            }

            /* propagation already set */
            if (proc_params->prop != HYD_ENV_PROP_UNSET) {
                HYDU_Error_printf("Duplicate propagation setting; previously set to %d\n",
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

                    status = HYDU_Env_create(&env, env_name, NULL, HYD_ENV_STATIC, 0);
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

            status = HYDU_Env_create(&env, env_name, env_value, HYD_ENV_STATIC, 0);
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
                HYDU_Error_printf("Duplicate setting for number of processes; previously set to %d\n",
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
     *    3. Specified through the environment HYDRA_USE_LOCALHOST
     */
    if (handle.host_file == NULL && getenv("HYDRA_HOST_FILE"))
        handle.host_file = MPIU_Strdup(getenv("HYDRA_HOST_FILE"));
    if (handle.host_file == NULL && getenv("HYDRA_USE_LOCALHOST"))
        handle.host_file = MPIU_Strdup("HYDRA_USE_LOCALHOST");
    if (handle.host_file == NULL) {
        HYDU_Error_printf("Host file not specified\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    proc_params = handle.proc_params;
    while (proc_params) {
        if (proc_params->exec[0] == NULL) {
            HYDU_Error_printf("no executable specified\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }

        if (proc_params->exec_proc_count == 0)
            proc_params->exec_proc_count = 1;

        proc_params = proc_params->next;
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
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_Print("mpiexec options:\n\n");
    HYDU_Print("  Debug level: %d\n", handle.debug);
    HYDU_Print("  Enable X: %d\n", handle.enablex);
    HYDU_Print("  Working dir: %s\n", handle.wdir);

    HYDU_Print("\n");
    HYDU_Print("  Global environment:\n");
    for (env = handle.global_env; env; env = env->next)
        HYDU_Print("    %s=%s (type: %s)\n", env->env_name, env->env_value,
                   HYDU_Env_type_str(env->env_type));

    HYDU_Print("\n");
    HYDU_Print("  Hydra internal environment:\n");
    for (env = handle.system_env; env; env = env->next)
        HYDU_Print("    %s=%s (type: %s)\n", env->env_name, env->env_value,
                   HYDU_Env_type_str(env->env_type));

    HYDU_Print("\n");
    HYDU_Print("  User set environment:\n");
    for (env = handle.user_env; env; env = env->next)
        HYDU_Print("    %s=%s (type: %s)\n", env->env_name, env->env_value,
                   HYDU_Env_type_str(env->env_type));

    HYDU_Print("\n");

    HYDU_FUNC_EXIT();
    return status;
}
