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

#define INCREMENT_ARGV(status)             \
    {                                      \
	if (!(++argv)) {                   \
	    (status) = HYD_INTERNAL_ERROR; \
	    goto fn_fail;                  \
	}                                  \
    }

static void show_version(void)
{
    printf("MPICH2 Version: %s\n", MPICH2_VERSION);
    printf("Process Manager: HYDRA\n");
    printf("Boot-strap servers available: %s\n", HYDRA_BSS_NAMES);
}


HYD_Status HYD_LCHI_get_parameters(char **t_argv)
{
    int i;
    char **argv = t_argv;
    char *env_name, *env_value, *str[4] = { NULL }, *progname = *argv;
    HYD_Env_t *env;
    struct HYD_Exec_info *exec_info;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_LCHU_init_params();

    status = HYDU_list_global_env(&handle.global_env);
    HYDU_ERR_POP(status, "unable to get the global env list\n");

    while (++argv && *argv) {

        if (!strcmp(*argv, "-h") || !strcmp(*argv, "-help") || !strcmp(*argv, "--help"))
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "");

        if (!strcmp(*argv, "--version")) {
            show_version();
            HYDU_ERR_SETANDJUMP(status, HYD_GRACEFUL_ABORT, "");
        }

        if (!strcmp(*argv, "--verbose")) {
            HYDU_ERR_CHKANDJUMP(status, handle.debug != -1, HYD_INTERNAL_ERROR,
                                "duplicate debug level\n");
            handle.debug = 1;
            continue;
        }

        if (!strcmp(*argv, "--enable-x") || !strcmp(*argv, "--disable-x")) {
            HYDU_ERR_CHKANDJUMP(status, handle.enablex != -1, HYD_INTERNAL_ERROR,
                                "duplicate enable-x\n");
            handle.enablex = !strcmp(*argv, "--enable-x");
            continue;
        }

        if (!strcmp(*argv, "-genvall")) {
            HYDU_ERR_CHKANDJUMP(status, handle.prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate prop setting\n");
            handle.prop = HYD_ENV_PROP_ALL;
            continue;
        }

        if (!strcmp(*argv, "-genvnone")) {
            HYDU_ERR_CHKANDJUMP(status, handle.prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate prop setting\n");
            handle.prop = HYD_ENV_PROP_NONE;
            continue;
        }

        if (!strcmp(*argv, "-genvlist")) {
            HYDU_ERR_CHKANDJUMP(status, handle.prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate prop setting\n");
            handle.prop = HYD_ENV_PROP_LIST;

            INCREMENT_ARGV(status);
            HYDU_comma_list_to_env_list(*argv, &handle.user_env);
            continue;
        }

        if (!strcmp(*argv, "-genv")) {
            INCREMENT_ARGV(status);
            env_name = MPIU_Strdup(*argv);
            INCREMENT_ARGV(status);
            env_value = MPIU_Strdup(*argv);

            status = HYDU_env_create(&env, env_name, env_value);
            HYDU_ERR_POP(status, "unable to create env struct\n");

            HYDU_append_env_to_list(*env, &handle.user_env);
            continue;
        }

        if (!strcmp(*argv, "-envall")) {
            status = HYD_LCHU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_ERR_CHKANDJUMP(status, exec_info->prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate prop setting\n");
            exec_info->prop = HYD_ENV_PROP_ALL;
            continue;
        }

        if (!strcmp(*argv, "-envnone")) {
            status = HYD_LCHU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_ERR_CHKANDJUMP(status, exec_info->prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate prop setting\n");
            exec_info->prop = HYD_ENV_PROP_NONE;
            continue;
        }

        if (!strcmp(*argv, "-envlist")) {
            status = HYD_LCHU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_ERR_CHKANDJUMP(status, exec_info->prop != HYD_ENV_PROP_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate prop setting\n");
            exec_info->prop = HYD_ENV_PROP_LIST;

            INCREMENT_ARGV(status);
            HYDU_comma_list_to_env_list(*argv, &exec_info->user_env);
            continue;
        }

        if (!strcmp(*argv, "-env")) {
            INCREMENT_ARGV(status);
            env_name = MPIU_Strdup(*argv);
            INCREMENT_ARGV(status);
            env_value = MPIU_Strdup(*argv);

            status = HYDU_env_create(&env, env_name, env_value);
            HYDU_ERR_POP(status, "unable to create env struct\n");

            status = HYD_LCHU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_append_env_to_list(*env, &exec_info->user_env);
            continue;
        }

        if (!strcmp(*argv, "-wdir")) {
            INCREMENT_ARGV(status);
            handle.wdir = MPIU_Strdup(*argv);
            continue;
        }

        if (!strcmp(*argv, "-n") || !strcmp(*argv, "-np")) {
            INCREMENT_ARGV(status);

            status = HYD_LCHU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            if (exec_info->exec_proc_count != 0)
                HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "duplicate proc count\n");

            exec_info->exec_proc_count = atoi(*argv);
            continue;
        }

        if (!strcmp(*argv, "-f")) {
            INCREMENT_ARGV(status);
            handle.host_file = MPIU_Strdup(*argv);
            continue;
        }

        /* The below options allow for "--foo=x" form of argument,
         * instead of "--foo x" for convenience. */
        status = HYDU_strsplit(*argv, &str[0], &str[1], '=');
        HYDU_ERR_POP(status, "string break returned error\n");

        if (!strcmp(str[0], "--bootstrap")) {
            if (!str[1]) {
                INCREMENT_ARGV(status);
                str[1] = *argv;
            }
            HYDU_ERR_CHKANDJUMP(status, handle.bootstrap, HYD_INTERNAL_ERROR,
                                "duplicate bootstrap server\n");
            handle.bootstrap = MPIU_Strdup(str[1]);
            continue;
        }

        if (!strcmp(str[0], "--binding")) {
            if (!str[1]) {
                INCREMENT_ARGV(status);
                str[1] = *argv;
            }
            HYDU_ERR_CHKANDJUMP(status, handle.binding != HYD_BIND_UNSET,
                                HYD_INTERNAL_ERROR, "duplicate binding\n");
            if (!strcmp(str[1], "none"))
                handle.binding = HYD_BIND_NONE;
            else if (!strcmp(str[1], "rr"))
                handle.binding = HYD_BIND_RR;
            else if (!strcmp(str[1], "buddy"))
                handle.binding = HYD_BIND_BUDDY;
            else if (!strcmp(str[1], "pack"))
                handle.binding = HYD_BIND_PACK;
            else {
                /* Check if the user wants to specify her own mapping */
                status = HYDU_strsplit(str[1], &str[2], &str[3], ':');
                HYDU_ERR_POP(status, "string break returned error\n");

                if (!strcmp(str[2], "user")) {
                    handle.binding = HYD_BIND_USER;
                    if (str[3])
                        handle.user_bind_map = MPIU_Strdup(str[3]);
                }
            }

            continue;
        }

        if (!strcmp(str[0], "--proxy-port")) {
            if (!str[1]) {
                INCREMENT_ARGV(status);
                str[1] = *argv;
            }
            HYDU_ERR_CHKANDJUMP(status, handle.proxy_port != -1, HYD_INTERNAL_ERROR,
                                "duplicate proxy port\n");
            handle.proxy_port = atoi(str[1]);
            continue;
        }

        if (*argv[0] == '-')
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized argument\n");

        status = HYD_LCHU_get_current_exec_info(&exec_info);
        HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

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
            exec_info->exec[i] = MPIU_Strdup(*argv);
            exec_info->exec[i + 1] = NULL;
        } while (++argv && *argv);

        if (!(*argv))
            break;

        continue;
    }

    if (handle.debug == -1)
        handle.debug = 0;

    if (handle.exec_info_list == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no local options set\n");

    if (handle.wdir == NULL) {
        HYDU_MALLOC(handle.wdir, char *, HYDRA_MAX_PATH, status);
        if (getcwd(handle.wdir, HYDRA_MAX_PATH) == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "allocated space is too small for absolute path\n");
    }

    if (handle.bootstrap == NULL && getenv("HYDRA_BOOTSTRAP"))
        handle.bootstrap = MPIU_Strdup(getenv("HYDRA_BOOTSTRAP"));
    if (handle.bootstrap == NULL)
        handle.bootstrap = MPIU_Strdup(HYDRA_DEFAULT_BSS);

    if (handle.host_file == NULL && getenv("HYDRA_HOST_FILE"))
        handle.host_file = MPIU_Strdup(getenv("HYDRA_HOST_FILE"));
    if (handle.host_file == NULL)
        handle.host_file = MPIU_Strdup("HYDRA_USE_LOCALHOST");

    status = HYDU_get_base_path(progname, handle.wdir, &handle.base_path);
    HYDU_ERR_POP(status, "unable to get base path\n");

    if (handle.binding == HYD_BIND_UNSET)
        handle.binding = HYD_BIND_NONE;

    if (handle.prop == HYD_ENV_PROP_UNSET)
        handle.prop = HYD_ENV_PROP_ALL;

    /* Check if any individual app has an environment preference */
    for (exec_info = handle.exec_info_list; exec_info; exec_info = exec_info->next) {
        if (exec_info->exec[0] == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        if (exec_info->exec_proc_count == 0)
            exec_info->exec_proc_count = 1;

        /* If no local env property is set, use the global one */
        if (exec_info->prop == HYD_ENV_PROP_UNSET)
            exec_info->prop = handle.prop;
    }

    if (handle.proxy_port == -1)
        handle.proxy_port = HYD_DEFAULT_PROXY_PORT;

  fn_exit:
    for (i = 0; i < 4; i++)
        if (str[i])
            HYDU_FREE(str[i]);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
