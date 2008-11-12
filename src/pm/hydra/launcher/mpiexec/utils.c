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
#include "csi.h"

HYD_CSI_Handle csi_handle;

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

#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "allocate_proc_params"
static HYD_Status allocate_proc_params(struct HYD_CSI_Proc_params **params)
{
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(proc_params, struct HYD_CSI_Proc_params *, sizeof(struct HYD_CSI_Proc_params), status);

    proc_params->user_num_procs = 0;
    proc_params->total_num_procs = 0;
    proc_params->total_proc_list = NULL;
    proc_params->total_core_list = NULL;

    proc_params->host_file = NULL;

    proc_params->exec[0] = NULL;
    proc_params->user_env = NULL;
    proc_params->prop = HYD_CSI_PROP_ENVNONE;
    proc_params->prop_env = NULL;
    proc_params->out = NULL;
    proc_params->err = NULL;
    proc_params->stdout_cb = NULL;
    proc_params->stderr_cb = NULL;
    proc_params->exit_status = NULL;
    proc_params->next = NULL;

    *params = proc_params;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "get_current_proc_params"
static HYD_Status get_current_proc_params(struct HYD_CSI_Proc_params **params)
{
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (csi_handle.proc_params == NULL) {
        status = allocate_proc_params(&csi_handle.proc_params);
        if (status != HYD_SUCCESS) {
            HYDU_Error_printf("unable to allocate proc_params\n");
            goto fn_fail;
        }
    }

    proc_params = csi_handle.proc_params;
    while (proc_params->next)
        proc_params = proc_params->next;

    *params = proc_params;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


#if defined FUNCNAME
#undef FUNCNAME
#endif /* FUNCNAME */
#define FUNCNAME "HYD_LCHI_Get_parameters"
HYD_Status HYD_LCHI_Get_parameters(int t_argc, char **t_argv)
{
    int argc = t_argc, i, got_hostfile;
    char **argv = t_argv;
    int local_params_started;
    char *arg;
    char *env_name, *env_value;
    HYDU_Env_t *env;
    struct HYD_CSI_Proc_params *proc_params;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    csi_handle.debug = -1;
    csi_handle.enablex = -1;
    csi_handle.wdir = NULL;

    status = HYDU_Env_global_list(&csi_handle.global_env);
    if (status != HYD_SUCCESS) {
        HYDU_Error_printf("unable to get the global env list\n");
        goto fn_fail;
    }
    csi_handle.system_env = NULL;
    csi_handle.user_env = NULL;
    csi_handle.prop = HYD_CSI_PROP_ENVNONE;
    csi_handle.prop_env = NULL;

    csi_handle.proc_params = NULL;

    local_params_started = 0;
    while (--argc && ++argv) {

        /* Check what debug level is requested */
        if (!strcmp(*argv, "-v") || !strcmp(*argv, "-vv") || !strcmp(*argv, "-vvv")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);
            CHECK_NEXT_ARG_VALID(status);

            /* Debug level already set */
            if (csi_handle.debug != -1) {
                HYDU_Error_printf("Duplicate debug level setting; previously set to %d\n",
                                  csi_handle.debug);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            if (!strcmp(*argv, "-v"))
                csi_handle.debug = 1;
            else if (!strcmp(*argv, "-vv"))
                csi_handle.debug = 2;
            else if (!strcmp(*argv, "-vvv"))
                csi_handle.debug = 3;

            continue;
        }

        /* Check if X forwarding is explicitly set */
        if (!strcmp(*argv, "--enable-x") || !strcmp(*argv, "--disable-x")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* X forwarding already enabled or disabled */
            if (csi_handle.enablex != -1) {
                HYDU_Error_printf("Duplicate enable-x setting; previously set to %d\n",
                                  csi_handle.enablex);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            csi_handle.enablex = !strcmp(*argv, "--enable-x");
            continue;
        }

        /* Check what all environment variables need to be propagated */
        if (!strcmp(*argv, "-genvall") || !strcmp(*argv, "-genvnone") || !strcmp(*argv, "-genvlist")) {
            CHECK_LOCAL_PARAM_START(local_params_started, status);

            /* propagation already set */
            if (csi_handle.prop != HYD_CSI_PROP_ENVNONE) {
                HYDU_Error_printf("Duplicate propagation setting; previously set to %d\n",
                                  csi_handle.prop);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            if (!strcmp(*argv, "-genvall")) {
                csi_handle.prop = HYD_CSI_PROP_ENVALL;
            }
            else if (!strcmp(*argv, "-genvnone")) {
                csi_handle.prop = HYD_CSI_PROP_ENVNONE;
            }
            else if (!strcmp(*argv, "-genvlist")) {
                csi_handle.prop = HYD_CSI_PROP_ENVLIST;
                CHECK_NEXT_ARG_VALID(status);
                do {
                    env_name = strtok(*argv, ",");
                    if (env_name == NULL)
                        break;

                    status = HYDU_Env_create(&env, env_name, NULL, HYDU_ENV_STATIC, 0);
                    if (status != HYD_SUCCESS) {
                        HYDU_Error_printf("unable to create env struct\n");
                        goto fn_fail;
                    }

                    status = HYDU_Env_add_to_list(&csi_handle.prop_env, *env);
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
            if (proc_params->prop != HYD_CSI_PROP_ENVNONE) {
                HYDU_Error_printf("Duplicate propagation setting; previously set to %d\n",
                                  proc_params->prop);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            if (!strcmp(*argv, "-envall")) {
                proc_params->prop = HYD_CSI_PROP_ENVALL;
            }
            else if (!strcmp(*argv, "-envnone")) {
                proc_params->prop = HYD_CSI_PROP_ENVNONE;
            }
            else if (!strcmp(*argv, "-envlist")) {
                proc_params->prop = HYD_CSI_PROP_ENVLIST;
                CHECK_NEXT_ARG_VALID(status);
                do {
                    env_name = strtok(*argv, ",");
                    if (env_name == NULL)
                        break;

                    status = HYDU_Env_create(&env, env_name, NULL, HYDU_ENV_STATIC, 0);
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

            status = HYDU_Env_create(&env, env_name, env_value, HYDU_ENV_STATIC, 0);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("unable to create env struct\n");
                goto fn_fail;
            }

            if (!strcmp(*argv, "-genv"))
                HYDU_Env_add_to_list(&csi_handle.user_env, *env);
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
            csi_handle.wdir = MPIU_Strdup(*argv);
        }

        if (!strcmp(*argv, "-n")) {
            local_params_started = 1;
            CHECK_NEXT_ARG_VALID(status);

            status = get_current_proc_params(&proc_params);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("get_current_proc_params returned error\n");
                goto fn_fail;
            }

            /* Num_procs already set */
            if (proc_params->user_num_procs != 0) {
                HYDU_Error_printf("Duplicate setting for number of processes; previously set to %d\n",
                                  proc_params->user_num_procs);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            proc_params->user_num_procs = atoi(*argv);

            continue;
        }

        if (!strcmp(*argv, "-f")) {
            local_params_started = 1;
            CHECK_NEXT_ARG_VALID(status);

            status = get_current_proc_params(&proc_params);
            if (status != HYD_SUCCESS) {
                HYDU_Error_printf("get_current_proc_params returned error\n");
                goto fn_fail;
            }

            /* host_file already set */
            if (proc_params->host_file != NULL) {
                HYDU_Error_printf("Duplicate setting for host file; previously set to %s\n",
                                  proc_params->host_file);
                status = HYD_INTERNAL_ERROR;
                goto fn_fail;
            }

            proc_params->host_file = MPIU_Strdup(*argv);
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
    if (csi_handle.debug == -1)
        csi_handle.debug = 0;

    /* We need at least one local exec */
    if (csi_handle.proc_params == NULL) {
        HYDU_Error_printf("No local options set\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    /* If wdir is not set, use the current one */
    if (csi_handle.wdir == NULL) {
        csi_handle.wdir = MPIU_Strdup(getcwd(NULL, 0));
    }

    proc_params = csi_handle.proc_params;
    got_hostfile = 0;
    while (proc_params) {
        if (proc_params->exec[0] == NULL) {
            HYDU_Error_printf("no executable specified\n");
            status = HYD_INTERNAL_ERROR;
            goto fn_fail;
        }

        if (proc_params->user_num_procs == 0)
            proc_params->user_num_procs = 1;

        if (proc_params->host_file == NULL && got_hostfile == 0 && getenv("HYDRA_HOST_FILE"))
            proc_params->host_file = MPIU_Strdup(getenv("HYDRA_HOST_FILE"));
        if (proc_params->host_file == NULL && got_hostfile == 0 && getenv("HYDRA_USE_LOCALHOST"))
            proc_params->host_file = MPIU_Strdup("HYDRA_USE_LOCALHOST");
        if (proc_params->host_file != NULL)
            got_hostfile = 1;

        proc_params = proc_params->next;
    }

    if (got_hostfile == 0) {
        HYDU_Error_printf("Host file not specified\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
