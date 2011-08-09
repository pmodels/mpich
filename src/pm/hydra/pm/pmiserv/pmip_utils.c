/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmip.h"
#include "bsci.h"
#include "topo.h"
#include "ckpoint.h"
#include "demux.h"
#include "hydra.h"

struct HYD_pmcd_pmip HYD_pmcd_pmip;

void HYD_pmcd_pmip_kill_localprocs(void)
{
    int i;

    /* Send the kill signal to all processes */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pid[i] != -1) {
            kill(HYD_pmcd_pmip.downstream.pid[i], SIGTERM);
            kill(HYD_pmcd_pmip.downstream.pid[i], SIGKILL);
        }
    }

    HYD_pmcd_pmip.downstream.forced_cleanup = 1;
}

static HYD_status control_port_fn(char *arg, char ***argv)
{
    char *port = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_pmcd_pmip.upstream.server_name, HYD_INTERNAL_ERROR,
                        "duplicate control port setting\n");

    port = HYDU_strdup(**argv);
    HYD_pmcd_pmip.upstream.server_name = HYDU_strdup(strtok(port, ":"));
    HYD_pmcd_pmip.upstream.server_port = atoi(strtok(NULL, ":"));

    (*argv)++;

  fn_exit:
    if (port)
        HYDU_FREE(port);
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status proxy_id_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.local.id, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status pgid_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.local.pgid, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status debug_fn(char *arg, char ***argv)
{
    return HYDU_set_int(arg, &HYD_pmcd_pmip.user_global.debug, 1);
}

static HYD_status rmk_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.rmk, **argv);

    (*argv)++;

    return status;
}

static HYD_status launcher_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.launcher, **argv);

    (*argv)++;

    return status;
}

static HYD_status launcher_exec_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.launcher_exec, **argv);

    (*argv)++;

    return status;
}

static HYD_status demux_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.demux, **argv);

    (*argv)++;

    return status;
}

static HYD_status iface_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.iface, **argv);

    (*argv)++;

    return status;
}

static HYD_status auto_cleanup_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.user_global.auto_cleanup, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status retries_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.local.retries, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status pmi_port_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.system_global.pmi_port, **argv);

    (*argv)++;

    return status;
}

static HYD_status pmi_fd_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.system_global.pmi_fd, **argv);

    (*argv)++;

    return status;
}

static HYD_status pmi_rank_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.system_global.pmi_rank, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status pmi_kvsname_fn(char *arg, char ***argv)
{
    HYDU_snprintf(HYD_pmcd_pmip.local.kvs->kvs_name, PMI_MAXKVSLEN, "%s", **argv);
    (*argv)++;

    return HYD_SUCCESS;
}

static HYD_status pmi_spawner_kvsname_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC(HYD_pmcd_pmip.local.spawner_kvs_name, char *, PMI_MAXKVSLEN, status);

    HYDU_snprintf(HYD_pmcd_pmip.local.spawner_kvs_name, PMI_MAXKVSLEN, "%s", **argv);
    (*argv)++;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_process_mapping_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.system_global.pmi_process_mapping, **argv);

    (*argv)++;

    return status;
}

static HYD_status binding_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.binding, **argv);

    (*argv)++;

    return status;
}

static HYD_status topolib_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.topolib, **argv);

    (*argv)++;

    return status;
}

static HYD_status ckpointlib_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.ckpointlib, **argv);

    (*argv)++;

    return status;
}

static HYD_status ckpoint_num_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.user_global.ckpoint_num, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status parse_ckpoint_prefix(char *pathlist)
{
    int i, prefixes;
    char *dummy;
    HYD_status status = HYD_SUCCESS;

    /* Find the number of prefixes provided */
    prefixes = 1;
    for (i = 0; pathlist[i]; i++)
        if (pathlist[i] == ':')
            prefixes++;

    /* Add one more to the prefix list for a NULL ending string */
    prefixes++;

    HYDU_MALLOC(HYD_pmcd_pmip.local.ckpoint_prefix_list, char **, prefixes * sizeof(char *),
                status);

    dummy = strtok(pathlist, ":");
    i = 0;
    while (dummy) {
        HYD_pmcd_pmip.local.ckpoint_prefix_list[i] = HYDU_strdup(dummy);
        dummy = strtok(NULL, ":");
        i++;
    }
    HYD_pmcd_pmip.local.ckpoint_prefix_list[i] = NULL;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status ckpoint_prefix_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.ckpoint_prefix, **argv);
    HYDU_ERR_POP(status, "error setting checkpoint prefix\n");

    status = parse_ckpoint_prefix(**argv);
    HYDU_ERR_POP(status, "error setting checkpoint prefix\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status global_env_fn(char *arg, char ***argv)
{
    int i, count;
    char *str;
    HYD_status status = HYD_SUCCESS;

    count = atoi(**argv);
    for (i = 0; i < count; i++) {
        (*argv)++;
        str = **argv;

        /* Environment variables are quoted; remove them */
        if (*str == '\'') {
            str++;
            str[strlen(str) - 1] = 0;
        }

        if (!strcmp(arg, "global-inherited-env"))
            HYDU_append_env_str_to_list(str, &HYD_pmcd_pmip.user_global.global_env.inherited);
        else if (!strcmp(arg, "global-system-env"))
            HYDU_append_env_str_to_list(str, &HYD_pmcd_pmip.user_global.global_env.system);
        else if (!strcmp(arg, "global-user-env"))
            HYDU_append_env_str_to_list(str, &HYD_pmcd_pmip.user_global.global_env.user);
    }
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status genv_prop_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.global_env.prop, **argv);

    (*argv)++;

    return status;
}

static HYD_status split_map(char *map, int *left, int *current, int *right)
{
    char *tmp;
    HYD_status status = HYD_SUCCESS;

    tmp = strtok(map, ",");
    HYDU_ASSERT(tmp, status);
    *left = atoi(tmp);

    tmp = strtok(NULL, ",");
    HYDU_ASSERT(tmp, status);
    *current = atoi(tmp);

    tmp = strtok(NULL, ",");
    HYDU_ASSERT(tmp, status);
    *right = atoi(tmp);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status global_core_map_fn(char *arg, char ***argv)
{
    char *map;
    HYD_status status = HYD_SUCCESS;

    /* Split the core map into three different segments */
    map = HYDU_strdup(**argv);
    HYDU_ASSERT(map, status);

    status = split_map(map, &HYD_pmcd_pmip.system_global.global_core_map.left,
                       &HYD_pmcd_pmip.system_global.global_core_map.current,
                       &HYD_pmcd_pmip.system_global.global_core_map.right);
    HYDU_ERR_POP(status, "unable to split the provided mapping\n");
    HYDU_FREE(map);

    HYD_pmcd_pmip.system_global.global_core_map.total =
        HYD_pmcd_pmip.system_global.global_core_map.left +
        HYD_pmcd_pmip.system_global.global_core_map.current +
        HYD_pmcd_pmip.system_global.global_core_map.right;

    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status filler_process_map_fn(char *arg, char ***argv)
{
    char *map;
    HYD_status status = HYD_SUCCESS;

    /* Split the core map into three different segments */
    map = HYDU_strdup(**argv);
    HYDU_ASSERT(map, status);

    status = split_map(map, &HYD_pmcd_pmip.system_global.filler_process_map.left,
                       &HYD_pmcd_pmip.system_global.filler_process_map.current,
                       &HYD_pmcd_pmip.system_global.filler_process_map.right);
    HYDU_ERR_POP(status, "unable to split the provided mapping\n");
    HYDU_FREE(map);

    HYD_pmcd_pmip.system_global.filler_process_map.total =
        HYD_pmcd_pmip.system_global.filler_process_map.left +
        HYD_pmcd_pmip.system_global.filler_process_map.current +
        HYD_pmcd_pmip.system_global.filler_process_map.right;

    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status global_process_count_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.system_global.global_process_count,
                          atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status version_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    if (strcmp(**argv, HYDRA_VERSION)) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "UI version string does not match proxy version\n");
    }
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status iface_ip_env_name_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.local.iface_ip_env_name, **argv);

    (*argv)++;

    return status;
}

static HYD_status hostname_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.local.hostname, **argv);

    (*argv)++;

    return status;
}

static HYD_status local_binding_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.local.local_binding, **argv);

    (*argv)++;

    return status;
}

static HYD_status proxy_core_count_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.local.proxy_core_count, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status exec_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    if (HYD_pmcd_pmip.exec_list == NULL) {
        status = HYDU_alloc_exec(&HYD_pmcd_pmip.exec_list);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");
    }
    else {
        for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);
        status = HYDU_alloc_exec(&exec->next);
        HYDU_ERR_POP(status, "unable to allocate proxy exec\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status exec_appnum_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);
    status = HYDU_set_int(arg, &exec->appnum, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status exec_proc_count_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);
    status = HYDU_set_int(arg, &exec->proc_count, atoi(**argv));

    (*argv)++;

    return status;
}

static HYD_status exec_local_env_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    int i, count;
    char *str;
    HYD_status status = HYD_SUCCESS;

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "NULL argument to exec local env\n");

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    count = atoi(**argv);
    for (i = 0; i < count; i++) {
        (*argv)++;
        str = **argv;

        /* Environment variables are quoted; remove them */
        if (*str == '\'') {
            str++;
            str[strlen(str) - 1] = 0;
        }
        HYDU_append_env_str_to_list(str, &exec->user_env);
    }
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status exec_env_prop_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    status = HYDU_set_str(arg, &exec->env_prop, **argv);

    (*argv)++;

    return status;
}

static HYD_status exec_wdir_fn(char *arg, char ***argv)
{
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    status = HYDU_set_str(arg, &exec->wdir, **argv);

    (*argv)++;

    return status;
}

static HYD_status exec_args_fn(char *arg, char ***argv)
{
    int i, count;
    struct HYD_exec *exec = NULL;
    HYD_status status = HYD_SUCCESS;

    if (**argv == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "NULL argument to exec args\n");

    for (exec = HYD_pmcd_pmip.exec_list; exec->next; exec = exec->next);

    count = atoi(**argv);
    for (i = 0; i < count; i++) {
        (*argv)++;
        exec->exec[i] = HYDU_strdup(**argv);
    }
    exec->exec[i] = NULL;
    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

struct HYD_arg_match_table HYD_pmcd_pmip_match_table[] = {
    /* Proxy parameters */
    {"control-port", control_port_fn, NULL},
    {"proxy-id", proxy_id_fn, NULL},
    {"pgid", pgid_fn, NULL},
    {"debug", debug_fn, NULL},
    {"rmk", rmk_fn, NULL},
    {"launcher", launcher_fn, NULL},
    {"launcher-exec", launcher_exec_fn, NULL},
    {"demux", demux_fn, NULL},
    {"iface", iface_fn, NULL},
    {"auto-cleanup", auto_cleanup_fn, NULL},
    {"retries", retries_fn, NULL},

    /* Executable parameters */
    {"pmi-port", pmi_port_fn, NULL},
    {"pmi-fd", pmi_fd_fn, NULL},
    {"pmi-rank", pmi_rank_fn, NULL},
    {"pmi-kvsname", pmi_kvsname_fn, NULL},
    {"pmi-spawner-kvsname", pmi_spawner_kvsname_fn, NULL},
    {"pmi-process-mapping", pmi_process_mapping_fn, NULL},
    {"binding", binding_fn, NULL},
    {"topolib", topolib_fn, NULL},
    {"ckpointlib", ckpointlib_fn, NULL},
    {"ckpoint-prefix", ckpoint_prefix_fn, NULL},
    {"ckpoint-num", ckpoint_num_fn, NULL},
    {"global-inherited-env", global_env_fn, NULL},
    {"global-system-env", global_env_fn, NULL},
    {"global-user-env", global_env_fn, NULL},
    {"genv-prop", genv_prop_fn, NULL},
    {"global-core-map", global_core_map_fn, NULL},
    {"filler-process-map", filler_process_map_fn, NULL},
    {"global-process-count", global_process_count_fn, NULL},
    {"version", version_fn, NULL},
    {"iface-ip-env-name", iface_ip_env_name_fn, NULL},
    {"hostname", hostname_fn, NULL},
    {"local-binding", local_binding_fn, NULL},
    {"proxy-core-count", proxy_core_count_fn, NULL},
    {"exec", exec_fn, NULL},
    {"exec-appnum", exec_appnum_fn, NULL},
    {"exec-proc-count", exec_proc_count_fn, NULL},
    {"exec-local-env", exec_local_env_fn, NULL},
    {"exec-env-prop", exec_env_prop_fn, NULL},
    {"exec-wdir", exec_wdir_fn, NULL},
    {"exec-args", exec_args_fn, NULL}
};

HYD_status HYD_pmcd_pmip_get_params(char **t_argv)
{
    char **argv = t_argv;
    static char dbg_prefix[2 * MAX_HOSTNAME_LEN];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    argv++;
    do {
        /* Get the proxy arguments  */
        status = HYDU_parse_array(&argv, HYD_pmcd_pmip_match_table);
        HYDU_ERR_POP(status, "error parsing input array\n");

        /* No more arguments left */
        if (!(*argv))
            break;
    } while (1);

    /* Verify the arguments we got */
    if (HYD_pmcd_pmip.upstream.server_name == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "server name not available\n");

    if (HYD_pmcd_pmip.upstream.server_port == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "server port not available\n");

    if (HYD_pmcd_pmip.user_global.demux == NULL)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "demux engine not available\n");

    if (HYD_pmcd_pmip.user_global.debug == -1)
        HYD_pmcd_pmip.user_global.debug = 0;

    status = HYDT_bsci_init(HYD_pmcd_pmip.user_global.rmk,
                            HYD_pmcd_pmip.user_global.launcher,
                            HYD_pmcd_pmip.user_global.launcher_exec,
                            0 /* disable x */ , HYD_pmcd_pmip.user_global.debug);
    HYDU_ERR_POP(status, "proxy unable to initialize bootstrap server\n");

    if (HYD_pmcd_pmip.local.id == -1) {
        /* We didn't get a proxy ID during launch; query the launcher
         * for it. */
        status = HYDT_bsci_query_proxy_id(&HYD_pmcd_pmip.local.id);
        HYDU_ERR_POP(status, "unable to query launcher for proxy ID\n");
    }

    if (HYD_pmcd_pmip.system_global.jobid == NULL) {
        status = HYDT_bsci_query_jobid(&HYD_pmcd_pmip.system_global.jobid);
        HYDU_ERR_POP(status, "unable to query launcher for job ID\n");
    }

    if (HYD_pmcd_pmip.local.id == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "proxy ID not available\n");

    if (HYD_pmcd_pmip.local.pgid == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PG ID not available\n");

    if (HYD_pmcd_pmip.local.retries == -1)
        HYD_pmcd_pmip.local.retries = 0;

    HYDU_dbg_finalize();
    HYDU_snprintf(dbg_prefix, 2 * MAX_HOSTNAME_LEN, "proxy:%d:%d",
                  HYD_pmcd_pmip.local.pgid, HYD_pmcd_pmip.local.id);
    status = HYDU_dbg_init((const char *) dbg_prefix);
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
