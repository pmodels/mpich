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

static HYD_status cmd_response(int fd, int pid, struct PMIU_cmd *pmi)
{
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PMI_RESPONSE;
    hdr.u.pmi.pid = pid;
    return HYD_pmcd_pmi_send(fd, pmi, &hdr, HYD_server_info.user_global.debug);
}

static HYD_status bcast_keyvals(int fd, int pid)
{
    int keyval_count, arg_count, j;
    struct HYD_pmcd_pmi_kvs_pair *run;
    struct HYD_proxy *proxy, *tproxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    /* find the number of keyvals */
    keyval_count = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next)
        keyval_count++;

    keyval_count -= pg_scratch->keyval_dist_count;

    struct PMIU_cmd pmi;
    if (keyval_count) {
        PMIU_cmd_init(&pmi, 1, "keyval_cache");
        arg_count = 1;
        for (run = pg_scratch->kvs->key_pair, j = 0; run; run = run->next, j++) {
            if (j < pg_scratch->keyval_dist_count)
                continue;

            PMIU_cmd_add_str(&pmi, run->key, run->val);

            arg_count++;
            if (arg_count >= MAX_PMI_ARGS) {
                pg_scratch->keyval_dist_count += (arg_count - 1);
                for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
                    status = cmd_response(tproxy->control_fd, pid, &pmi);
                    HYDU_ERR_POP(status, "error writing PMI line\n");
                }

                PMIU_cmd_init(&pmi, 1, "keyval_cache");
                arg_count = 1;
            }
        }

        if (arg_count > 1) {
            pg_scratch->keyval_dist_count += (arg_count - 1);
            for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
                status = cmd_response(tproxy->control_fd, pid, &pmi);
                HYDU_ERR_POP(status, "error writing PMI line\n");
            }
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_in(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy, *tproxy;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    proxy->pg->barrier_count++;
    if (proxy->pg->barrier_count == proxy->pg->proxy_count) {
        proxy->pg->barrier_count = 0;

        bcast_keyvals(fd, pid);

        struct PMIU_cmd pmi_response;
        PMIU_cmd_init(&pmi_response, 1, "barrier_out");
        for (tproxy = proxy->pg->proxy_list; tproxy; tproxy = tproxy->next) {
            status = cmd_response(tproxy->control_fd, pid, &pmi_response);
            HYDU_ERR_POP(status, "error writing PMI line\n");
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* NOTE: this is an aggregate put from proxy with multiple key=val pairs */
static HYD_status fn_put(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    /* FIXME: leak of pmi's abstraction */
    for (int i = 0; i < pmi->num_tokens; i++) {
        int ret;
        status = HYD_pmcd_pmi_add_kvs(pmi->tokens[i].key, pmi->tokens[i].val,
                                      pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add key pair to kvs\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *kvsname, *key, *val;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    kvsname = PMIU_cmd_find_keyval(pmi, "kvsname");
    HYDU_ERR_CHKANDJUMP(status, kvsname == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: kvsname\n");

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    val = NULL;
    if (!strcmp(key, "PMI_dead_processes")) {
        val = pg_scratch->dead_processes;
        goto found_val;
    }

    if (strcmp(pg_scratch->kvs->kvsname, kvsname))
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "kvsname (%s) does not match this group's kvs space (%s)\n",
                            kvsname, pg_scratch->kvs->kvsname);

    /* Try to find the key */
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            val = run->val;
            break;
        }
    }

    struct PMIU_cmd pmi_response;
  found_val:
    PMIU_cmd_init(&pmi_response, 1, "get_result");
    if (val) {
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
        PMIU_cmd_add_str(&pmi_response, "value", val);
    } else {
        static char tmp[100];
        MPL_snprintf(tmp, 100, "key_%s_not_found", key);

        PMIU_cmd_add_str(&pmi_response, "rc", "-1");
        PMIU_cmd_add_str(&pmi_response, "msg", tmp);
        PMIU_cmd_add_str(&pmi_response, "value", "unknown");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_spawn(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_proxy *proxy;
    struct HYD_exec *exec_list = NULL, *exec;
    struct HYD_env *env;
    struct HYD_node *node;

    char key[PMI_MAXKEYLEN];
    const char *val;
    int nprocs, preput_num, info_num, ret;
    char *execname, *path = NULL;

    int i, k, new_pgid;
    int argcnt;
    struct HYD_string_stash proxy_stash;
    char *control_port;
    struct HYD_string_stash stash;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Initialize the proxy stash, so it can be freed if we jump to
     * exit */
    HYD_STRING_STASH_INIT(proxy_stash);


    /* With PMI-v1, PMI_Spawn_multiple comes as separate mcmd=spawn
     * commands. We use static variables (pg, execlist) to sync between
     * segments.
     *   * In the first segment, we allocate pg and execlist.
     *   * For each segment, we allocate, parse, and enqueue the exec object.
     *   * In the last segment, we execute the spawn.
     */

    int total_spawns, spawnssofar;
    val = PMIU_cmd_find_keyval(pmi, "totspawns");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: totspawns\n");
    total_spawns = atoi(val);

    val = PMIU_cmd_find_keyval(pmi, "spawnssofar");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: totspawns\n");
    spawnssofar = atoi(val);

    static struct HYD_pg *spawn_pg = NULL;
    static struct HYD_exec *spawn_exec_list = NULL;

    if (spawnssofar == 1) {
        /* Allocate a new process group */
        for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);
        new_pgid = pg->pgid + 1;

        status = HYDU_alloc_pg(&pg->next, new_pgid);
        HYDU_ERR_POP(status, "unable to allocate process group\n");

        pg = pg->next;
        spawn_pg = pg;

        proxy = HYD_pmcd_pmi_find_proxy(fd);
        HYDU_ASSERT(proxy, status);

        pg->spawner_pg = proxy->pg;

        status = HYDU_alloc_exec(&exec_list);
        HYDU_ERR_POP(status, "unable to allocate exec\n");
        exec_list->appnum = 0;
        spawn_exec_list = exec_list;

        exec = exec_list;
    } else {
        pg = spawn_pg;
        new_pgid = pg->pgid;
        exec_list = spawn_exec_list;

        for (exec = exec_list; exec->next; exec = exec->next);
        status = HYDU_alloc_exec(&exec->next);
        HYDU_ERR_POP(status, "unable to allocate exec\n");
        exec->next->appnum = exec->appnum + 1;
        exec = exec->next;
    }
    HYDU_ASSERT(pg, status);

    {
        /* For each segment, we create an exec structure */
        val = PMIU_cmd_find_keyval(pmi, "nprocs");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: nprocs\n");
        nprocs = atoi(val);
        pg->pg_process_count += nprocs;

        val = PMIU_cmd_find_keyval(pmi, "argcnt");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: argcnt\n");
        argcnt = atoi(val);

        val = PMIU_cmd_find_keyval(pmi, "info_num");
        if (val)
            info_num = atoi(val);
        else
            info_num = 0;

        /* Info keys */
        for (i = 0; i < info_num; i++) {
            const char *info_key, *info_val;

            MPL_snprintf(key, PMI_MAXKEYLEN, "info_key_%d", i);
            val = PMIU_cmd_find_keyval(pmi, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_key = val;

            MPL_snprintf(key, PMI_MAXKEYLEN, "info_val_%d", i);
            val = PMIU_cmd_find_keyval(pmi, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_val = val;

            if (!strcmp(info_key, "path")) {
                path = MPL_strdup(info_val);
            } else if (!strcmp(info_key, "wdir")) {
                exec->wdir = MPL_strdup(info_val);
            } else if (!strcmp(info_key, "host") || !strcmp(info_key, "hosts")) {
                char *val_copy = MPL_strdup(info_val);
                char *saveptr;
                char *host = strtok_r(val_copy, ",", &saveptr);
                while (host) {
                    status = HYDU_process_mfile_token(host, 1, &pg->user_node_list);
                    HYDU_ERR_POP(status, "error creating node list\n");
                    host = strtok_r(NULL, ",", &saveptr);
                }
                MPL_free(val_copy);
            } else if (!strcmp(info_key, "hostfile")) {
                pg->user_node_list = NULL;
                status = HYDU_parse_hostfile(info_val, &pg->user_node_list,
                                             HYDU_process_mfile_token);
                HYDU_ERR_POP(status, "error parsing hostfile\n");
            } else {
                /* Unrecognized info key; ignore */
            }
        }

        status = HYDU_correct_wdir(&exec->wdir);
        HYDU_ERR_POP(status, "unable to correct wdir\n");

        val = PMIU_cmd_find_keyval(pmi, "execname");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: execname\n");
        if (path == NULL)
            execname = MPL_strdup(val);
        else {
            HYD_STRING_STASH_INIT(stash);
            HYD_STRING_STASH(stash, path, status);
            HYD_STRING_STASH(stash, MPL_strdup("/"), status);
            HYD_STRING_STASH(stash, MPL_strdup(val), status);

            HYD_STRING_SPIT(stash, execname, status);
        }

        i = 0;
        exec->exec[i++] = execname;
        for (k = 0; k < argcnt; k++) {
            MPL_snprintf(key, PMI_MAXKEYLEN, "arg%d", k + 1);
            val = PMIU_cmd_find_keyval(pmi, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            exec->exec[i++] = MPL_strdup(val);
        }
        exec->exec[i++] = NULL;

        exec->proc_count = nprocs;

        /* It is not clear what kind of environment needs to get
         * passed to the spawned process. Don't set anything here, and
         * let the proxy do whatever it does by default. */
        exec->env_prop = NULL;

        status = HYDU_env_create(&env, "PMI_SPAWNED", "1");
        HYDU_ERR_POP(status, "unable to create PMI_SPAWNED environment\n");
        exec->user_env = env;
    }

    if (spawnssofar < total_spawns) {
        goto fn_exit;
    }

    status = HYD_pmcd_pmi_alloc_pg_scratch(pg);
    HYDU_ERR_POP(status, "unable to allocate pg scratch space\n");

    pg->pg_process_count = 0;
    for (exec = exec_list; exec; exec = exec->next)
        pg->pg_process_count += exec->proc_count;

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* Get the common keys and deal with them */
    /* NOTE: with PMI-v1, common keys are repeated in each exec segment;
     *       here we take it from the last segment and ignore from others.
     */
    val = PMIU_cmd_find_keyval(pmi, "preput_num");
    if (val)
        preput_num = atoi(val);
    else
        preput_num = 0;

    for (i = 0; i < preput_num; i++) {
        const char *preput_key, *preput_val;

        MPL_snprintf(key, PMI_MAXKEYLEN, "preput_key_%d", i);
        val = PMIU_cmd_find_keyval(pmi, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_key = val;

        MPL_snprintf(key, PMI_MAXKEYLEN, "preput_val_%d", i);
        val = PMIU_cmd_find_keyval(pmi, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_val = val;

        status = HYD_pmcd_pmi_add_kvs(preput_key, preput_val, pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add key pair to kvs\n");
    }

    /* Create the proxy list */
    if (pg->user_node_list) {
        status = HYDU_create_proxy_list(exec_list, pg->user_node_list, pg, false);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    } else {
        status = HYDU_create_proxy_list(exec_list, HYD_server_info.node_list, pg, false);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    }
    HYDU_free_exec_list(exec_list);

    if (pg->user_node_list) {
        pg->pg_core_count = 0;
        for (i = 0, node = pg->user_node_list; node; node = node->next, i++)
            pg->pg_core_count += node->core_count;
    } else {
        pg->pg_core_count = 0;
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            pg->pg_core_count += proxy->node->core_count;
    }

    status = HYDU_sock_create_and_listen_portstr(HYD_server_info.user_global.iface,
                                                 HYD_server_info.localhost,
                                                 HYD_server_info.port_range, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) new_pgid);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    /* Go to the last PG */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);

    status = HYD_pmcd_pmi_fill_in_proxy_args(&proxy_stash, control_port, new_pgid);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");
    MPL_free(control_port);

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pg);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_bsci_launch_procs(proxy_stash.strlist, pg->proxy_list, HYD_FALSE, NULL);
    HYDU_ERR_POP(status, "launcher cannot launch processes\n");

    {
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init(&pmi_response, 1, "spawn_result");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");

        status = cmd_response(fd, pid, &pmi_response);
        HYDU_ERR_POP(status, "error writing PMI line\n");
    }

    /* Cache the pre-initialized keyvals on the new proxies */
    if (preput_num)
        bcast_keyvals(fd, pid);

  fn_exit:
    HYD_STRING_STASH_FREE(proxy_stash);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_publish_name(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *val;
    char *name = NULL, *port = NULL;
    int success = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((val = PMIU_cmd_find_keyval(pmi, "service")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: service\n");
    name = MPL_strdup(val);

    if ((val = PMIU_cmd_find_keyval(pmi, "port")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: port\n");
    port = MPL_strdup(val);

    status = HYD_pmcd_pmi_publish(name, port, &success);
    HYDU_ERR_POP(status, "error publishing service\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "publish_result");
    if (success) {
        PMIU_cmd_add_str(&pmi_response, "info", "ok");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
    } else {
        PMIU_cmd_add_str(&pmi_response, "info", "ok");
        PMIU_cmd_add_str(&pmi_response, "rc", "1");
        PMIU_cmd_add_str(&pmi_response, "msg", "key_already_present");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    if (name)
        MPL_free(name);
    if (port)
        MPL_free(port);

    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_unpublish_name(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *name;
    int success = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((name = PMIU_cmd_find_keyval(pmi, "service")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: service\n");

    status = HYD_pmcd_pmi_unpublish(name, &success);
    HYDU_ERR_POP(status, "error unpublishing service\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "unpublish_result");
    if (success) {
        PMIU_cmd_add_str(&pmi_response, "info", "ok");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
    } else {
        PMIU_cmd_add_str(&pmi_response, "info", "ok");
        PMIU_cmd_add_str(&pmi_response, "rc", "1");
        PMIU_cmd_add_str(&pmi_response, "msg", "service_not_found");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_lookup_name(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *name, *value = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if ((name = PMIU_cmd_find_keyval(pmi, "service")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: service\n");

    status = HYD_pmcd_pmi_lookup(name, &value);
    HYDU_ERR_POP(status, "error while looking up service\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "lookup_result");
    if (value) {
        PMIU_cmd_add_str(&pmi_response, "port", value);
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
    } else {
        PMIU_cmd_add_str(&pmi_response, "rc", "1");
        PMIU_cmd_add_str(&pmi_response, "msg", "service_not_found");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    if (value)
        MPL_free(value);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_abort(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    /* set a default exit code of 1 */
    int exitcode = 1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (PMIU_cmd_find_keyval(pmi, "exitcode") == NULL)
        HYDU_ERR_POP(status, "cannot find token: exitcode\n");

    exitcode = strtol(PMIU_cmd_find_keyval(pmi, "exitcode"), NULL, 10);

  fn_exit:
    /* clean everything up and exit */
    status = HYDT_bsci_wait_for_completion(0);
    exit(exitcode);

    /* never get here */
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


/* TODO: abort, create_kvs, destroy_kvs, getbyidx */
static struct HYD_pmcd_pmi_handle pmi_v1_handle_fns_foo[] = {
    {"barrier_in", fn_barrier_in},
    {"put", fn_put},
    {"get", fn_get},
    {"spawn", fn_spawn},
    {"publish_name", fn_publish_name},
    {"unpublish_name", fn_unpublish_name},
    {"lookup_name", fn_lookup_name},
    {"abort", fn_abort},
    {"\0", NULL}
};

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v1 = pmi_v1_handle_fns_foo;
