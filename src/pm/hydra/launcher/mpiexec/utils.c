/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "lchu.h"

#define HYDRA_MAX_PATH 4096

extern HYD_Handle handle;

#define INCREMENT_ARGV(status)             \
    {                                      \
	if (!(++argv)) {                   \
	    (status) = HYD_INTERNAL_ERROR; \
	    goto fn_fail;                  \
	}                                  \
    }

static void show_version(void)
{
    printf("HYDRA Build Details\n");
    printf("Process Manager: PMI\n");
    printf("Boot-strap servers available: %s\n", HYDRA_BSS_NAMES);
}


HYD_Status HYD_LCHI_get_parameters(char **t_argv)
{
    int i, local_env_set;
    char **argv = t_argv, *tmp;
    char *env_name, *env_value, *str[4] = { NULL, NULL, NULL, NULL }, *progname = *argv;
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
        
        if(!strcmp(*argv, "--boot-proxies")) {
            /* FIXME: Prevent usage of incompatible params */
            handle.bootstrap = HYDU_strdup("ssh");
            handle.is_proxy_launcher = 1;
            handle.prop = HYD_ENV_PROP_ALL;
            continue;
        }

        if(!strcmp(*argv, "--remote-proxy")) {
            /* FIXME: We should get rid of this option eventually.
             * This should be the default case. The centralized
             * version should use an option like "--local-proxy"
             */
            handle.is_proxy_remote = 1;
            handle.prop = HYD_ENV_PROP_ALL;
            continue;
        }

        if(!strcmp(*argv, "--shutdown-proxies")) {
            handle.is_proxy_remote = 1;
            handle.is_proxy_terminator = 1;
            handle.prop = HYD_ENV_PROP_ALL;
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
            env_name = HYDU_strdup(*argv);
            INCREMENT_ARGV(status);
            env_value = HYDU_strdup(*argv);

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
            env_name = HYDU_strdup(*argv);
            INCREMENT_ARGV(status);
            env_value = HYDU_strdup(*argv);

            status = HYDU_env_create(&env, env_name, env_value);
            HYDU_ERR_POP(status, "unable to create env struct\n");

            status = HYD_LCHU_get_current_exec_info(&exec_info);
            HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

            HYDU_append_env_to_list(*env, &exec_info->user_env);
            continue;
        }

        if (!strcmp(*argv, "-wdir")) {
            INCREMENT_ARGV(status);
            handle.wdir = HYDU_strdup(*argv);
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
            handle.host_file = HYDU_strdup(*argv);
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
            handle.bootstrap = HYDU_strdup(str[1]);
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
                        handle.user_bind_map = HYDU_strdup(str[3]);
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

        if (!strcmp(str[0], "--pproxy-port")) {
            if (!str[1]) {
                INCREMENT_ARGV(status);
                str[1] = *argv;
            }
            HYDU_ERR_CHKANDJUMP(status, handle.pproxy_port != -1, HYD_INTERNAL_ERROR,
                                "duplicate persistent proxy port\n");
            handle.pproxy_port = atoi(str[1]);
            continue;
        }

        if (*argv[0] == '-')
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized argument\n");

        status = HYD_LCHU_get_current_exec_info(&exec_info);
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
    /* In the case of the proxy launcher, aka --boot-proxies, there is no executable
     * specified */
    if(handle.is_proxy_launcher || handle.is_proxy_terminator) {
        if(handle.exec_info_list != NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                "No executable should be specified when booting proxies\n");

        status = HYD_LCHU_get_current_exec_info(&exec_info);
        HYDU_ERR_POP(status, "get_current_exec_info returned error\n");

        exec_info->exec[0] = HYDU_strdup(HYD_PROXY_NAME" --persistent");
        exec_info->exec[1] = NULL;
        exec_info->exec_proc_count = 1;

        env_name = HYDU_strdup("HYD_PROXY_PORT");
        env_value = HYDU_int_to_str(handle.pproxy_port);

        status = HYDU_env_create(&env, env_name, env_value);
        HYDU_ERR_POP(status, "unable to create env struct\n");

        HYDU_append_env_to_list(*env, &exec_info->user_env);
    }


    tmp = getenv("MPIEXEC_DEBUG");
    if (handle.debug == -1 && tmp)
        handle.debug = atoi(tmp) ? 1 : 0;
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

    tmp = getenv("HYDRA_BOOTSTRAP");
    if (handle.bootstrap == NULL && tmp)
        handle.bootstrap = HYDU_strdup(tmp);
    if (handle.bootstrap == NULL)
        handle.bootstrap = HYDU_strdup(HYDRA_DEFAULT_BSS);

    tmp = getenv("HYDRA_HOST_FILE");
    if (handle.host_file == NULL && tmp)
        handle.host_file = HYDU_strdup(tmp);
    if (handle.host_file == NULL)
        handle.host_file = HYDU_strdup("HYDRA_USE_LOCALHOST");

    status = HYDU_get_base_path(progname, handle.wdir, &handle.base_path);
    HYDU_ERR_POP(status, "unable to get base path\n");

    tmp = getenv("HYDRA_BINDING");
    if (handle.binding == HYD_BIND_UNSET && tmp)
        handle.binding = !strcmp(tmp, "none") ? HYD_BIND_NONE :
            !strcmp(tmp, "rr") ? HYD_BIND_RR :
            !strcmp(tmp, "buddy") ? HYD_BIND_BUDDY :
            !strcmp(tmp, "pack") ? HYD_BIND_PACK :
            HYD_BIND_USER;
    if (handle.binding == HYD_BIND_UNSET)
        handle.binding = HYD_BIND_NONE;

    /* Check environment for setting the global environment */
    tmp = getenv("HYDRA_ENV");
    if (handle.prop == HYD_ENV_PROP_UNSET && tmp)
        handle.prop = !strcmp(tmp, "all") ? HYD_ENV_PROP_ALL : HYD_ENV_PROP_NONE;

    /* Make sure local executable is set */
    local_env_set = 0;
    for (exec_info = handle.exec_info_list; exec_info; exec_info = exec_info->next) {
        if (exec_info->exec[0] == NULL)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "no executable specified\n");

        if (exec_info->exec_proc_count == 0)
            exec_info->exec_proc_count = 1;

        if (exec_info->prop != HYD_ENV_PROP_UNSET)
            local_env_set = 1;
    }

    /* If no global or local environment is set, use the default */
    if ((handle.prop == HYD_ENV_PROP_UNSET) && (local_env_set == 0))
        handle.prop = HYD_ENV_PROP_ALL;

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
