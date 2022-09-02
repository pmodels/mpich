/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "mpiexec.h"
#include "uiu.h"
#include "pmi_util.h"   /* from libpmi, for PMIU_verbose */

/* The order of loading options:
 *     * set default sentinel values
 *     * command line
 *     * config file
 *     * environment
 *     * reset sentinels to default
 *
 * Because the latter should not override the former, we use sentinel values
 * to tell whether an options is already set.
 */

static void init_ui_mpich_info(void);
static HYD_status check_environment(void);
static void set_default_values(void);
static HYD_status process_config_token(char *token, int newline, void *data);
static HYD_status parse_args(char **t_argv, int reading_config_file);
static HYD_status post_process(void);

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
    status = parse_args(argv, 0);
    HYDU_ERR_POP(status, "unable to parse user arguments\n");

    if (HYD_ui_mpich_info.config_file == NULL) {
        /* Check if there's a config file location in the environment */
        ret = MPL_env2str("HYDRA_CONFIG_FILE", &env_file);
        if (ret) {
            ret = open(env_file, O_RDONLY);
            if (ret >= 0) {
                close(ret);
                HYD_ui_mpich_info.config_file = MPL_strdup(env_file);
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
            } else {
                close(ret);
                HYD_ui_mpich_info.config_file = conf_file;
                goto config_file_check_exit;
            }
        }

        /* Check if there's a config file in the hard-coded location */
        conf_file = MPL_strdup(HYDRA_CONF_FILE);
        HYDU_ERR_CHKANDJUMP(status, NULL == conf_file, HYD_INTERNAL_ERROR, "strdup failed\n");
        ret = open(conf_file, O_RDONLY);
        if (ret < 0) {
            MPL_free(conf_file);
        } else {
            close(ret);
            HYD_ui_mpich_info.config_file = conf_file;
            goto config_file_check_exit;
        }
    }

  config_file_check_exit:
    if (HYD_ui_mpich_info.config_file) {
        char **config_argv;
        HYDU_MALLOC_OR_JUMP(config_argv, char **, HYD_NUM_TMP_STRINGS * sizeof(char *), status);

        status =
            HYDU_parse_hostfile(HYD_ui_mpich_info.config_file, config_argv, process_config_token);
        HYDU_ERR_POP(status, "error parsing config file\n");

        status = parse_args(config_argv, 1);
        HYDU_ERR_POP(status, "error parsing config args\n");

        MPL_free(HYD_ui_mpich_info.config_file);
        HYD_ui_mpich_info.config_file = NULL;
    }

    /* Get the base path */
    /* Find the last '/' in the executable name */
    post = MPL_strdup(progname);
    HYDU_ERR_CHKANDJUMP(status, NULL == post, HYD_INTERNAL_ERROR, "strdup failed\n");
    loc = strrchr(post, '/');
    if (!loc) { /* If there is no path */
        HYD_server_info.base_path = NULL;
        status = HYDU_find_in_path(progname, &HYD_server_info.base_path);
        HYDU_ERR_POP(status, "error while searching for executable in the user path\n");
    } else {    /* There is a path */
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
        } else {        /* absolute */
            HYD_server_info.base_path = MPL_strdup(post);
        }
    }
    MPL_free(post);

    status = check_environment();
    HYDU_ERR_POP(status, "checking environment variables\n");

    set_default_values();

    if (HYD_server_info.user_global.debug) {
        PMIU_verbose = 1;
    }

    status = post_process();
    HYDU_ERR_POP(status, "post processing\n");

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

/* ---- internal routines ---- */

static void init_ui_mpich_info(void)
{
    HYD_ui_mpich_info.ppn = -1;
    HYD_ui_mpich_info.timeout = -1;
    HYD_ui_mpich_info.print_all_exitcodes = -1;
    HYD_ui_mpich_info.sort_order = NONE;
    HYD_ui_mpich_info.output_from = -1;
    HYD_ui_mpich_info.prepend_pattern = NULL;
    HYD_ui_mpich_info.outfile_pattern = NULL;
    HYD_ui_mpich_info.errfile_pattern = NULL;
    HYD_ui_mpich_info.config_file = NULL;
    HYD_ui_mpich_info.reading_config_file = 0;
    HYD_ui_mpich_info.hostname_propagation = -1;
}

static void set_default_values(void)
{
    if (HYD_ui_mpich_info.print_all_exitcodes == -1)
        HYD_ui_mpich_info.print_all_exitcodes = 0;

    if (HYD_server_info.enable_profiling == -1)
        HYD_server_info.enable_profiling = 0;

    if (HYD_server_info.user_global.debug == -1)
        HYD_server_info.user_global.debug = 0;

    if (HYD_server_info.user_global.topo_debug == -1)
        HYD_server_info.user_global.topo_debug = 0;

    if (HYD_server_info.user_global.auto_cleanup == -1)
        HYD_server_info.user_global.auto_cleanup = 1;

    /* Default universe size if the user did not specify anything is
     * INFINITE */
    if (HYD_server_info.user_global.usize == HYD_USIZE_UNSET)
        HYD_server_info.user_global.usize = HYD_USIZE_INFINITE;

    if (HYD_server_info.user_global.pmi_port == -1)
        HYD_server_info.user_global.pmi_port = 0;

    if (HYD_server_info.user_global.skip_launch_node == -1)
        HYD_server_info.user_global.skip_launch_node = 0;

    if (HYD_server_info.user_global.gpus_per_proc == HYD_GPUS_PER_PROC_UNSET)
        HYD_server_info.user_global.gpus_per_proc = HYD_GPUS_PER_PROC_AUTO;

    if (HYD_server_info.user_global.gpu_subdevs_per_proc == HYD_GPUS_PER_PROC_UNSET)
        HYD_server_info.user_global.gpu_subdevs_per_proc = HYD_GPUS_PER_PROC_AUTO;
}

/* In case a boolean environment value is unparsable (not 1|0|yes|no|true|false|on|off),
 * raise error. */
#define ENV2BOOL(name, var_ptr) \
    do { \
        if (-1 == MPL_env2bool(name, var_ptr)) { \
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unable to parse %s\n", name); \
        } \
    } while (0)

static HYD_status check_environment(void)
{
    char *tmp;
    HYD_status status = HYD_SUCCESS;

    if (HYD_server_info.user_global.debug == -1)
        ENV2BOOL("HYDRA_DEBUG", &HYD_server_info.user_global.debug);

    if (HYD_server_info.user_global.topo_debug == -1)
        ENV2BOOL("HYDRA_TOPO_DEBUG", &HYD_server_info.user_global.topo_debug);

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

    /* If hostname propagation is not set on the command-line, check
     * for the environment variable */
    if (HYD_ui_mpich_info.hostname_propagation == -1) {
        ENV2BOOL("HYDRA_HOSTNAME_PROPAGATION", &HYD_ui_mpich_info.hostname_propagation);
    }

    if (HYD_ui_mpich_info.timeout == -1) {
        MPL_env2int("MPIEXEC_TIMEOUT", &HYD_ui_mpich_info.timeout);
    }

    /* Check if the user wants us to use a port within a certain
     * range. */
    if (MPL_env2str("MPIR_CVAR_CH3_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_PARAM_CH3_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_CH3_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_CVAR_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_CVAR_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_PARAM_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIR_PARAM_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPICH_PORT_RANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIEXEC_PORTRANGE", (const char **) &HYD_server_info.port_range) ||
        MPL_env2str("MPIEXEC_PORT_RANGE", (const char **) &HYD_server_info.port_range)) {
        HYD_server_info.port_range = MPL_strdup(HYD_server_info.port_range);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status process_config_token(char *token, int newline, void *data)
{
    static int idx = 0;
    char **config_argv = data;

    if (idx && newline && strcmp(config_argv[idx - 1], ":")) {
        /* If this is a newline, but not the first one, and the
         * previous token was not a ":", add an executable delimiter
         * ':' */
        config_argv[idx++] = MPL_strdup(":");
    }

    config_argv[idx++] = MPL_strdup(token);
    assert(idx < HYD_NUM_TMP_STRINGS);
    config_argv[idx] = NULL;

    return HYD_SUCCESS;
}

static HYD_status parse_args(char **t_argv, int reading_config_file)
{
    char **argv = t_argv;
    struct HYD_exec *exec;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();
    HYD_ui_mpich_info.reading_config_file = reading_config_file;

    do {
        /* Get the mpiexec arguments  */
        status = HYDU_parse_array(&argv, HYD_mpiexec_match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        if (!(*argv))
            break;

        /* Get the executable information */
        /* Read the executable till you hit the end of a ":" */
        do {
            status = HYD_uii_get_current_exec(&exec);
            HYDU_ERR_POP(status, "HYD_uii_get_current_exec returned error\n");

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

static HYD_status post_process(void)
{
    HYD_status status = HYD_SUCCESS;

    for (struct HYD_exec * exec = HYD_uii_mpx_exec_list; exec; exec = exec->next) {
        if (HYD_server_info.hybrid_hosts) {
            /* Do not convert to abs path when hybrid_hosts option is set */
        } else {
            status = HYDU_correct_wdir(&exec->wdir);
            HYDU_ERR_POP(status, "unable to correct wdir\n");
        }
    }

    /* If an interface is provided, set that */
    if (HYD_server_info.user_global.iface) {
        if (HYD_ui_mpich_info.hostname_propagation == 1) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "cannot set iface and force hostname propagation");
        }

        HYDU_append_env_to_list("MPIR_CVAR_NEMESIS_TCP_NETWORK_IFACE",
                                HYD_server_info.user_global.iface,
                                &HYD_server_info.user_global.global_env.system);
    } else {
        /* If hostname propagation is requested (or not set), set the
         * environment variable for doing that */
        if (HYD_ui_mpich_info.hostname_propagation || HYD_ui_mpich_info.hostname_propagation == -1)
            HYD_server_info.iface_ip_env_name = MPL_strdup("MPIR_CVAR_CH3_INTERFACE_HOSTNAME");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
