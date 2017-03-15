/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmip.h"
#include "bsci.h"
#include "topo.h"
#include "ckpoint.h"
#include "demux.h"
#include "hydra.h"


void HYD_pmcd_pmip_send_signal(int sig)
{
    int i, pgid;

    /* Send the kill signal to all processes */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
        if (HYD_pmcd_pmip.downstream.pid[i] != -1) {
#if defined(HAVE_GETPGID) && defined(HAVE_SETSID)
            /* If we are able to get the process group ID, and the
             * child process has its own process group ID, send a
             * signal to the entire process group */
            pgid = getpgid(HYD_pmcd_pmip.downstream.pid[i]);
            killpg(pgid, sig);
#else
            kill(HYD_pmcd_pmip.downstream.pid[i], sig);
#endif
        }

    HYD_pmcd_pmip.downstream.forced_cleanup = 1;
}

HYD_status HYD_pmcd_pmi_fill_in_child_proxy_args(struct HYD_string_stash *proxy_stash , char *control_port, int pgid, int start_node_id)
{
    int i, arg = 0, use_ddd, use_valgrind;
    struct HYD_string_stash stash;
    char *str;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Hack to use ddd and valgrind with the proxy */
    if (MPL_env2bool("HYDRA_USE_DDD", &use_ddd) == 0)
        use_ddd = 0;
    if (MPL_env2bool("HYDRA_USE_VALGRIND", &use_valgrind) == 0)
        use_valgrind = 0;

    HYD_STRING_STASH_INIT(*proxy_stash);
    if (use_ddd)
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("ddd"), status);

    if (use_valgrind)
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("valgrind"), status);

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, MPL_strdup(HYD_pmcd_pmip.base_path), status);
    HYD_STRING_STASH(stash, MPL_strdup(HYDRA_PMI_PROXY), status);
    HYD_STRING_SPIT(stash, str, status);

    HYD_STRING_STASH(*proxy_stash, str, status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--control-port"), status);
    HYD_STRING_STASH(*proxy_stash, MPL_strdup(control_port), status);

    if (HYD_pmcd_pmip.user_global.debug == 1)
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--debug"), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--branch-count"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_pmcd_pmip.user_global.branch_count), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--parent-proxy-id"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_pmcd_pmip.local.id), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--launcher"), status);
    HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYD_pmcd_pmip.user_global.launcher), status);

    if (HYD_pmcd_pmip.user_global.launcher_exec) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--launcher-exec"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYD_pmcd_pmip.user_global.launcher_exec), status);
    }

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--demux"), status);
    HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYD_pmcd_pmip.user_global.demux), status);

    if (HYD_pmcd_pmip.user_global.iface) {
        HYD_STRING_STASH(*proxy_stash, MPL_strdup("--iface"), status);
        HYD_STRING_STASH(*proxy_stash, MPL_strdup(HYD_pmcd_pmip.user_global.iface), status);
    }

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--pgid"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(pgid), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--usize"), status);
    HYD_STRING_STASH(*proxy_stash, HYDU_int_to_str(HYD_pmcd_pmip.user_global.usize), status);

    HYD_STRING_STASH(*proxy_stash, MPL_strdup("--proxy-id"), status);

    if (use_ddd) {
        HYDU_dump_noprefix(stdout, "\nUse proxy launch args: ");
        HYDU_print_strlist(proxy_stash->strlist);
        HYDU_dump_noprefix(stdout, "\n");
    }

    if (HYD_pmcd_pmip.user_global.debug == 1) {
        HYDU_dump_noprefix(stdout, "\nProxy launch args: ");
        HYDU_print_strlist(proxy_stash->strlist);
        HYDU_dump_noprefix(stdout, "\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_send_procinfo_request(void)
{
    int sent, closed;
    struct HYD_pmcd_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = SEND_EXEC;
    hdr.buflen = 0;
    hdr.pid = HYD_pmcd_pmip.local.id;
    hdr.pmi_version = 1;
    hdr.rank = 0;
    /* We send command upstream */
    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send SEND_EXEC header upstream\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmci_wait_for_children_completion()
{
    int not_complete;
    HYD_status status = HYD_SUCCESS;
    int ncompleted = 0, *procs = NULL, *exit_statuses = NULL, i, j;

    HYDU_FUNC_ENTER();

    /* wait for the children to get back with the exit status */
    status = HYDT_bsci_wait_for_completion(-1, &ncompleted, &procs, &exit_statuses);
    HYDU_ERR_POP(status, "bootstrap server returned error waiting for completion\n");

    /* Make sure all the children have sent their exit status'es */
    not_complete = 1;
    while (not_complete) {
        int i;
        not_complete = 0;
        for (i = 0; i < HYD_pmcd_pmip.children.num; i++) {
            if (HYD_pmcd_pmip.children.exited[i] == 0) {
                not_complete = 1;
                break;
            }
            if (not_complete)
                break;
        }
        if (not_complete) {
            status = HYDT_dmx_wait_for_event(-1);
            HYDU_ERR_POP(status, "error waiting for demux event\n");
        }
    }

    /* check if some application processes unexpectedly completed
       (for example, with MPI_Abort) */
    if(ncompleted) {
        for(i = 0; i < ncompleted; i++) {
            for(j = 0; j < HYD_pmcd_pmip.local.proxy_process_count; j++) {
                if(procs[i] == HYD_pmcd_pmip.downstream.pid[j]) {
                    HYD_pmcd_pmip.downstream.exit_status[j] = exit_statuses[i];
                    break;
                }
            }
        }
    }

  fn_exit:
    if (procs)
        MPL_free(procs);
    if (exit_statuses)
        MPL_free(exit_statuses);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_status control_port_fn(char *arg, char ***argv)
{
    char *port = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_ERR_CHKANDJUMP(status, HYD_pmcd_pmip.upstream.server_name, HYD_INTERNAL_ERROR,
                        "duplicate control port setting\n");

    port = MPL_strdup(**argv);
    HYD_pmcd_pmip.upstream.server_name = MPL_strdup(strtok(port, ":"));
    HYD_pmcd_pmip.upstream.server_port = atoi(strtok(NULL, ":"));

    (*argv)++;

  fn_exit:
    if (port)
        MPL_free(port);
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

static HYD_status parent_proxy_id_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.local.parent_id, atoi(**argv));

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

static HYD_status usize_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYD_pmcd_pmip.user_global.usize = atoi(**argv);

    (*argv)++;

    return status;
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

static HYD_status branch_count_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.user_global.branch_count, atoi(**argv));
    HYDU_ERR_POP(status, "error setting branch count\n");

  fn_exit:
    (*argv)++;
    return status;

  fn_fail:
    goto fn_exit;
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

static HYD_status pmi_kvsname_fn(char *arg, char ***argv)
{
    MPL_snprintf(HYD_pmcd_pmip.local.kvs->kvsname, PMI_MAXKVSLEN, "%s", **argv);
    (*argv)++;

    return HYD_SUCCESS;
}

static HYD_status pmi_spawner_kvsname_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.local.spawner_kvsname, char *, PMI_MAXKVSLEN, status);

    MPL_snprintf(HYD_pmcd_pmip.local.spawner_kvsname, PMI_MAXKVSLEN, "%s", **argv);
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

static HYD_status mapping_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.mapping, **argv);

    (*argv)++;

    return status;
}

static HYD_status membind_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.membind, **argv);

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

    HYDU_MALLOC_OR_JUMP(HYD_pmcd_pmip.local.ckpoint_prefix_list, char **, prefixes * sizeof(char *),
                        status);

    dummy = strtok(pathlist, ":");
    i = 0;
    while (dummy) {
        HYD_pmcd_pmip.local.ckpoint_prefix_list[i] = MPL_strdup(dummy);
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

    HYDU_FUNC_EXIT();
    return status;
}

static HYD_status genv_prop_fn(char *arg, char ***argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_set_str(arg, &HYD_pmcd_pmip.user_global.global_env.prop, **argv);

    (*argv)++;

    return status;
}

static HYD_status global_core_map_fn(char *arg, char ***argv)
{
    char *map, *tmp;
    HYD_status status = HYD_SUCCESS;

    /* Split the core map into three different segments */
    map = MPL_strdup(**argv);
    HYDU_ASSERT(map, status);

    tmp = strtok(map, ",");
    HYDU_ASSERT(tmp, status);
    HYD_pmcd_pmip.system_global.global_core_map.local_filler = atoi(tmp);

    tmp = strtok(NULL, ",");
    HYDU_ASSERT(tmp, status);
    HYD_pmcd_pmip.system_global.global_core_map.local_count = atoi(tmp);

    tmp = strtok(NULL, ",");
    HYDU_ASSERT(tmp, status);
    HYD_pmcd_pmip.system_global.global_core_map.global_count = atoi(tmp);

    MPL_free(map);

    (*argv)++;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status pmi_id_map_fn(char *arg, char ***argv)
{
    char *map, *tmp;
    HYD_status status = HYD_SUCCESS;

    /* Split the core map into three different segments */
    map = MPL_strdup(**argv);
    HYDU_ASSERT(map, status);

    tmp = strtok(map, ",");
    HYDU_ASSERT(tmp, status);
    HYD_pmcd_pmip.system_global.pmi_id_map.filler_start = atoi(tmp);

    tmp = strtok(NULL, ",");
    HYDU_ASSERT(tmp, status);
    HYD_pmcd_pmip.system_global.pmi_id_map.non_filler_start = atoi(tmp);

    MPL_free(map);

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

    status = HYDU_set_int(arg, &HYD_pmcd_pmip.system_global.global_process_count, atoi(**argv));

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
        exec->exec[i] = MPL_strdup(**argv);
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
    {"parent-proxy-id", parent_proxy_id_fn, NULL},
    {"pgid", pgid_fn, NULL},
    {"debug", debug_fn, NULL},
    {"usize", usize_fn, NULL},
    {"rmk", rmk_fn, NULL},
    {"launcher", launcher_fn, NULL},
    {"launcher-exec", launcher_exec_fn, NULL},
    {"demux", demux_fn, NULL},
    {"iface", iface_fn, NULL},
    {"auto-cleanup", auto_cleanup_fn, NULL},
    {"retries", retries_fn, NULL},
    {"branch-count", branch_count_fn, NULL},

    /* Executable parameters */
    {"pmi-kvsname", pmi_kvsname_fn, NULL},
    {"pmi-spawner-kvsname", pmi_spawner_kvsname_fn, NULL},
    {"pmi-process-mapping", pmi_process_mapping_fn, NULL},
    {"topolib", topolib_fn, NULL},
    {"binding", binding_fn, NULL},
    {"mapping", mapping_fn, NULL},
    {"membind", membind_fn, NULL},
    {"ckpointlib", ckpointlib_fn, NULL},
    {"ckpoint-prefix", ckpoint_prefix_fn, NULL},
    {"ckpoint-num", ckpoint_num_fn, NULL},
    {"global-inherited-env", global_env_fn, NULL},
    {"global-system-env", global_env_fn, NULL},
    {"global-user-env", global_env_fn, NULL},
    {"genv-prop", genv_prop_fn, NULL},
    {"global-core-map", global_core_map_fn, NULL},
    {"pmi-id-map", pmi_id_map_fn, NULL},
    {"global-process-count", global_process_count_fn, NULL},
    {"version", version_fn, NULL},
    {"iface-ip-env-name", iface_ip_env_name_fn, NULL},
    {"hostname", hostname_fn, NULL},
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

    if (HYD_pmcd_pmip.local.id == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "proxy ID not available\n");

    if (HYD_pmcd_pmip.local.pgid == -1)
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PG ID not available\n");

    if (HYD_pmcd_pmip.local.retries == -1)
        HYD_pmcd_pmip.local.retries = 0;

    HYDU_dbg_finalize();
    if (HYD_pmcd_pmip.user_global.branch_count == -1)
        MPL_snprintf(dbg_prefix, 2 * MAX_HOSTNAME_LEN, "proxy:%d:%d",
                     HYD_pmcd_pmip.local.pgid, HYD_pmcd_pmip.local.id);
    else
        MPL_snprintf(dbg_prefix, 2 * MAX_HOSTNAME_LEN, "proxy:%d:%d:%d",
                     HYD_pmcd_pmip.local.pgid, HYD_pmcd_pmip.local.id, HYD_pmcd_pmip.local.parent_id);
    status = HYDU_dbg_init((const char *) dbg_prefix);
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status HYDU_alloc_pmi_r2f(struct HYD_pmi_ranks2fds **r2f)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC_OR_JUMP(*r2f, struct HYD_pmi_ranks2fds *, sizeof(struct HYD_pmi_ranks2fds), status);
    HYDU_ERR_POP(status, "Unable to alloc r2f table\n");

    (*r2f)->rank = -1;
    (*r2f)->fd = HYD_FD_UNSET;
    (*r2f)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_free_fd_list(struct HYD_pmi_ranks2fds **r2f_list)
{
    struct HYD_pmi_ranks2fds *next;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    while ( *r2f_list ){
        next = (*r2f_list)->next;
        MPL_free(*r2f_list);
        *r2f_list = next;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYDU_add_pmi_r2f(struct HYD_pmi_ranks2fds **r2f_list, int rank, int fd)
{
    struct HYD_pmi_ranks2fds *r2f, *prev;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ( *r2f_list == NULL ) {
        status = HYDU_alloc_pmi_r2f(r2f_list);
        HYDU_ERR_POP(status, "Unable to alloc PMI ranks2fds\n");
        (*r2f_list)->rank = rank;
        (*r2f_list)->fd = fd;
    } else {
        r2f = *r2f_list;
        while ( r2f ){
            if ( r2f->rank == rank ) {
                goto fn_exit;
            }
            prev = r2f;
            r2f = r2f->next;
        }
        status = HYDU_alloc_pmi_r2f(&r2f);
        HYDU_ERR_POP(status, "unable to alloc PMI ranks2fds\n");
        r2f->rank = rank;
        r2f->fd = fd;
        prev->next = r2f;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

int HYDU_get_fd_by_rank(struct HYD_pmi_ranks2fds **r2f_list, int rank)
{
    struct HYD_pmi_ranks2fds *r2f;

    if (*r2f_list == NULL) {
        return -1;
    } else {
        r2f=*r2f_list;
        while (r2f) {
            if (r2f->rank == rank)
                return r2f->fd;
            r2f=r2f->next;
        }
    }
    return -1;
}

int HYDU_get_rank_by_fd(struct HYD_pmi_ranks2fds **r2f_list, int fd)
{
    struct HYD_pmi_ranks2fds *r2f;

    if (*r2f_list == NULL) {
        return -1;
    } else {
        r2f=*r2f_list;
        while (r2f) {
            if (r2f->fd == fd)
                return r2f->rank;
            r2f=r2f->next;
        }
    }
    return -1;
}
