/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "hydra.h"
#include "bsci.h"
#include "pmiserv.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

static struct HYD_pg *spawn_pg = NULL;
static struct HYD_exec *spawn_exec_list = NULL;

static HYD_status allocate_spawn_pg(int fd);
static HYD_status fill_exec_params(struct PMIU_cmd *pmi, struct HYD_exec *exec, int j);
static HYD_status fill_preput_kvs(struct PMIU_cmd *pmi, struct HYD_pmcd_pmi_kvs *kvs);
static HYD_status do_spawn(void);

static char *get_exec_path(const char *execname, const char *path);
static HYD_status parse_info_hosts(const char *host_str, struct HYD_pg *pg);

HYD_status HYD_pmiserv_spawn(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    const char *thrid = NULL;

    if (pmi->version == 1) {
        /* With PMI-v1, PMI_Spawn_multiple comes as separate mcmd=spawn
         * commands. We use static variables (spawn_pg, spawn_exec_list) to sync between
         * segments.
         *   * In the first segment, we allocate pg and execlist.
         *   * For each segment, we allocate, parse, and enqueue the exec object.
         *   * In the last segment, we execute the spawn.
         */

        int total_spawns, spawnssofar;
        HYD_PMI_GET_INTVAL(pmi, "totspawns", total_spawns);
        HYD_PMI_GET_INTVAL(pmi, "spawnssofar", spawnssofar);

        struct HYD_exec *exec;
        if (spawnssofar == 1) {
            status = allocate_spawn_pg(fd);
            HYDU_ERR_POP(status, "spawn failed\n");

            /* NOTE: with PMI-v1, common keys are repeated in each exec segment;
             *       here we take it from the first segment and ignore from others.
             */
            struct HYD_pmcd_pmi_pg_scratch *pg_scratch = spawn_pg->pg_scratch;
            status = fill_preput_kvs(pmi, pg_scratch->kvs);
            HYDU_ERR_POP(status, "spawn failed\n");

            status = HYDU_alloc_exec(&spawn_exec_list);
            HYDU_ERR_POP(status, "unable to allocate exec\n");

            exec = spawn_exec_list;
            exec->appnum = 0;
        } else {
            HYDU_ASSERT(spawn_pg, status);

            for (exec = spawn_exec_list; exec->next; exec = exec->next);
            status = HYDU_alloc_exec(&exec->next);
            HYDU_ERR_POP(status, "unable to allocate exec\n");

            exec->next->appnum = exec->appnum + 1;
            exec = exec->next;
        }


        /* For each segment, we create an exec structure */
        status = fill_exec_params(pmi, exec, -1);
        HYDU_ERR_POP(status, "spawn failed\n");

        if (spawnssofar < total_spawns) {
            goto fn_exit;
        }
    } else {
        /* PMI-v2 */
        HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "thrid", thrid, NULL);

        status = allocate_spawn_pg(fd);
        HYDU_ERR_POP(status, "spawn failed\n");

        struct HYD_pmcd_pmi_pg_scratch *pg_scratch = spawn_pg->pg_scratch;
        status = fill_preput_kvs(pmi, pg_scratch->kvs);
        HYDU_ERR_POP(status, "spawn failed\n");

        int ncmds;
        HYD_PMI_GET_INTVAL(pmi, "ncmds", ncmds);

        struct HYD_exec *exec;
        exec = NULL;
        for (int j = 0; j < ncmds; j++) {
            if (exec == NULL) {
                status = HYDU_alloc_exec(&spawn_exec_list);
                HYDU_ERR_POP(status, "spawn failed\n");
                exec = spawn_exec_list;
                exec->appnum = j;
            } else {
                status = HYDU_alloc_exec(&exec->next);
                HYDU_ERR_POP(status, "spawn failed\n");
                exec = exec->next;
                exec->appnum = j;
            }

            status = fill_exec_params(pmi, exec, j);
            HYDU_ERR_POP(status, "spawn failed\n");
        }
    }

    status = do_spawn();
    HYDU_ERR_POP(status, "spawn failed\n");

    if (pmi->version == 1) {
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, 1, "spawn_result");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");

        status = HYD_pmiserv_pmi_reply(fd, pid, &pmi_response);
        HYDU_ERR_POP(status, "error writing PMI line\n");

        /* Cache the pre-initialized keyvals on the new proxies */
        HYD_pmiserv_bcast_keyvals(fd, pid);
    } else {
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, 2, "spawn-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        struct HYD_pmcd_pmi_pg_scratch *pg_scratch = spawn_pg->pg_scratch;
        PMIU_cmd_add_str(&pmi_response, "jobid", pg_scratch->kvs->kvsname);
        PMIU_cmd_add_str(&pmi_response, "nerrs", "0");;

        status = HYD_pmiserv_pmi_reply(fd, pid, &pmi_response);
        HYDU_ERR_POP(status, "send command failed\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* ---- internal routines ---- */

static HYD_status allocate_spawn_pg(int fd)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_pg *pg;
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);

    status = HYDU_alloc_pg(&pg->next, pg->pgid + 1);
    HYDU_ERR_POP(status, "unable to allocate process group\n");

    status = HYD_pmcd_pmi_alloc_pg_scratch(pg->next);
    HYDU_ERR_POP(status, "unable to allocate pg scratch space\n");

    pg = pg->next;
    spawn_pg = pg;

    struct HYD_proxy *proxy;
    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg->spawner_pg = proxy->pg;

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

static HYD_status fill_exec_params(struct PMIU_cmd *pmi, struct HYD_exec *exec, int j)
{
    HYD_status status = HYD_SUCCESS;

    struct HYD_pg *pg = spawn_pg;

    int nprocs, argcnt, info_num;
    const char *execname;
    if (pmi->version == 1) {
        HYD_PMI_GET_INTVAL(pmi, "nprocs", nprocs);
        HYD_PMI_GET_INTVAL(pmi, "argcnt", argcnt);
        HYD_PMI_GET_INTVAL_WITH_DEFAULT(pmi, "info_num", info_num, 0);
        HYD_PMI_GET_STRVAL(pmi, "execname", execname);
    } else {
        HYD_PMI_GET_INTVAL_J(pmi, "maxprocs", j, nprocs);
        HYD_PMI_GET_INTVAL_J(pmi, "argc", j, argcnt);
        HYD_PMI_GET_INTVAL_J_WITH_DEFAULT(pmi, "infokeycount", j, info_num, 0);
        HYD_PMI_GET_STRVAL_J(pmi, "subcmd", j, execname);
    }

    const char *path = NULL;
    /* Info keys */
    for (int i = 0; i < info_num; i++) {
        char key[100];

        const char *info_key, *info_val;
        if (pmi->version == 1) {
            MPL_snprintf(key, 100, "info_key_%d", i);
            HYD_PMI_GET_STRVAL(pmi, key, info_key);
            MPL_snprintf(key, 100, "info_val_%d", i);
            HYD_PMI_GET_STRVAL(pmi, key, info_val);
        } else {
            MPL_snprintf(key, 100, "infokey%d", i);
            HYD_PMI_GET_STRVAL_J(pmi, key, j, info_key);
            MPL_snprintf(key, 100, "infoval%d", i);
            HYD_PMI_GET_STRVAL_J(pmi, key, j, info_val);
        }

        if (!strcmp(info_key, "path")) {
            path = info_val;
        } else if (!strcmp(info_key, "wdir")) {
            exec->wdir = MPL_strdup(info_val);
        } else if (!strcmp(info_key, "host") || !strcmp(info_key, "hosts")) {
            status = parse_info_hosts(info_val, pg);
            HYDU_ERR_POP(status, "failed spawn\n");
        } else if (!strcmp(info_key, "hostfile")) {
            pg->user_node_list = NULL;
            status = HYDU_parse_hostfile(info_val, &pg->user_node_list, HYDU_process_mfile_token);
            HYDU_ERR_POP(status, "error parsing hostfile\n");
        } else {
            /* Unrecognized info key; ignore */
        }
    }

    status = HYDU_correct_wdir(&exec->wdir);
    HYDU_ERR_POP(status, "unable to correct wdir\n");

    exec->exec[0] = get_exec_path(execname, path);
    for (int i = 0; i < argcnt; i++) {
        char key[100];
        const char *arg;
        if (pmi->version == 1) {
            MPL_snprintf(key, 100, "arg%d", i + 1);
            HYD_PMI_GET_STRVAL(pmi, key, arg);
        } else {
            MPL_snprintf(key, 100, "argv%d", i);
            HYD_PMI_GET_STRVAL_J(pmi, key, j, arg);
        }
        exec->exec[i + 1] = MPL_strdup(arg);
    }
    exec->exec[argcnt + 1] = NULL;

    exec->proc_count = nprocs;

    /* It is not clear what kind of environment needs to get
     * passed to the spawned process. Don't set anything here, and
     * let the proxy do whatever it does by default. */
    exec->env_prop = NULL;

    struct HYD_env *env;
    status = HYDU_env_create(&env, "PMI_SPAWNED", "1");
    HYDU_ERR_POP(status, "unable to create PMI_SPAWNED environment\n");
    exec->user_env = env;

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

static HYD_status fill_preput_kvs(struct PMIU_cmd *pmi, struct HYD_pmcd_pmi_kvs *kvs)
{
    HYD_status status = HYD_SUCCESS;

    const char *key_preput_num;
    const char *fmt_preput_key;
    const char *fmt_preput_val;
    if (pmi->version == 1) {
        key_preput_num = "preput_num";
        fmt_preput_key = "preput_key_%d";
        fmt_preput_val = "preput_val_%d";
    } else {
        key_preput_num = "preputcount";
        fmt_preput_key = "ppkey%d";
        fmt_preput_val = "ppval%d";
    }

    int preput_num;
    HYD_PMI_GET_INTVAL_WITH_DEFAULT(pmi, key_preput_num, preput_num, 0);

    for (int i = 0; i < preput_num; i++) {
        char key[100];
        const char *preput_key, *preput_val;
        MPL_snprintf(key, 100, fmt_preput_key, i);
        HYD_PMI_GET_STRVAL(pmi, key, preput_key);
        MPL_snprintf(key, 100, fmt_preput_val, i);
        HYD_PMI_GET_STRVAL(pmi, key, preput_val);

        int ret;
        status = HYD_pmcd_pmi_add_kvs(preput_key, preput_val, kvs, &ret);
        HYDU_ERR_POP(status, "unable to add key pair to kvs\n");
    }

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}

static HYD_status do_spawn(void)
{
    HYD_status status = HYD_SUCCESS;

    struct HYD_pg *pg = spawn_pg;
    struct HYD_exec *exec_list = spawn_exec_list;

    /* Initialize the proxy stash, so it can be freed if we jump to exit */
    struct HYD_string_stash proxy_stash;
    HYD_STRING_STASH_INIT(proxy_stash);

    pg->pg_process_count = 0;
    for (struct HYD_exec * exec = exec_list; exec; exec = exec->next) {
        pg->pg_process_count += exec->proc_count;
    }

    /* PMI-v2 kvs-fence */
    status = HYD_pmiserv_epoch_init(pg);
    HYDU_ERR_POP(status, "unable to init epoch\n");

    /* Create the proxy list */
    if (pg->user_node_list) {
        status = HYDU_create_proxy_list(exec_list, pg->user_node_list, pg, false);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    } else {
        status = HYDU_create_proxy_list(exec_list, HYD_server_info.node_list, pg, false);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    }
    HYDU_free_exec_list(exec_list);

    pg->pg_core_count = 0;
    if (pg->user_node_list) {
        for (struct HYD_node * node = pg->user_node_list; node; node = node->next) {
            pg->pg_core_count += node->core_count;
        }
    } else {
        for (struct HYD_proxy * proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            pg->pg_core_count += proxy->node->core_count;
        }
    }

    int pgid = pg->pgid;

    char *control_port;
    status = HYDU_sock_create_and_listen_portstr(HYD_server_info.user_global.iface,
                                                 HYD_server_info.localhost,
                                                 HYD_server_info.port_range, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) pgid);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    /* Go to the last PG */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);

    status = HYD_pmcd_pmi_fill_in_proxy_args(&proxy_stash, control_port, pgid);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");
    MPL_free(control_port);

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pg);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_bsci_launch_procs(proxy_stash.strlist, pg->proxy_list, HYD_FALSE, NULL);
    HYDU_ERR_POP(status, "launcher cannot launch processes\n");

  fn_exit:
    HYD_STRING_STASH_FREE(proxy_stash);
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

/* utilities */

static char *get_exec_path(const char *execname, const char *path)
{
    if (path == NULL) {
        return MPL_strdup(execname);
    } else {
        char *buf;
        int len_path = strlen(path);
        int len_name = strlen(execname);
        int len = len_path + len_name + 2;
        buf = MPL_malloc(len, MPL_MEM_OTHER);
        if (buf) {
            strcpy(buf, path);
            strcpy(buf + len_path, "/");
            strcpy(buf + len_path + 1, execname);
        }
        return buf;
    }
}

static HYD_status parse_info_hosts(const char *host_str, struct HYD_pg *pg)
{
    HYD_status status = HYD_SUCCESS;

    pg->user_node_list = NULL;

    char *s = MPL_strdup(host_str);
    char *saveptr;
    char *host = strtok_r(s, ",", &saveptr);
    while (host) {
        status = HYDU_process_mfile_token(host, 1, &pg->user_node_list);
        HYDU_ERR_POP(status, "error creating node list\n");
        host = strtok_r(NULL, ",", &saveptr);
    }
    MPL_free(s);

  fn_exit:
    return status;
  fn_fail:
    goto fn_exit;
}
