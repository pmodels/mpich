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
#include "pmi_v2_common.h"

static HYD_status fn_info_getjobattr(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
static HYD_status fn_kvs_put(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
static HYD_status fn_kvs_get(int fd, int pid, int pgid, struct PMIU_cmd *pmi);
static HYD_status fn_kvs_fence(int fd, int pid, int pgid, struct PMIU_cmd *pmi);

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status cmd_response(int fd, int pid, struct PMIU_cmd *pmi)
{
    struct HYD_pmcd_hdr hdr;
    HYD_pmcd_init_header(&hdr);
    hdr.cmd = CMD_PMI_RESPONSE;
    hdr.u.pmi.pid = pid;
    return HYD_pmcd_pmi_send(fd, pmi, &hdr, HYD_server_info.user_global.debug);
}

static HYD_status poke_progress(const char *key)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *list_head = NULL, *list_tail = NULL;
    int i, count;
    HYD_status status = HYD_SUCCESS;

    for (count = 0, req = pending_reqs; req; req = req->next)
        count++;

    for (i = 0; i < count; i++) {
        /* Dequeue a request */
        req = pending_reqs;
        if (pending_reqs) {
            pending_reqs = pending_reqs->next;
            req->next = NULL;
        }

        if (key && req && strcmp(key, req->key)) {
            /* If the key doesn't match the request, just queue it back */
            if (list_head == NULL) {
                list_head = req;
                list_tail = req;
            } else {
                list_tail->next = req;
                req->prev = list_tail;
                list_tail = req;
            }
        } else {
            status = fn_kvs_get(req->fd, req->pid, req->pgid, req->pmi);
            HYDU_ERR_POP(status, "kvs_get returned error\n");

            /* Free the dequeued request */
            PMIU_cmd_free(req->pmi);
            MPL_free(req);
        }
    }

    if (list_head) {
        list_tail->next = pending_reqs;
        pending_reqs = list_head;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_getjobattr(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *key;
    const char *thrid;
    const char *val;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    val = NULL;
    if (!strcmp(key, "PMI_dead_processes"))
        val = pg_scratch->dead_processes;

    /* Try to find the key */
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            val = run->val;
            break;
        }
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "info-getjobattr-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    if (val) {
        PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
        PMIU_cmd_add_str(&pmi_response, "value", val);
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else {
        PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_put(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *key, *val, *thrid;
    int ret;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_v2_reqs *req;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    val = PMIU_cmd_find_keyval(pmi, "value");
    if (val == NULL) {
        /* the user sent an empty string */
        val = MPL_strdup("");
    }

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    status = HYD_pmcd_pmi_add_kvs(key, val, pg_scratch->kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "kvs-put-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_int(&pmi_response, "rc", ret);

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

    for (req = pending_reqs; req; req = req->next) {
        if (!strcmp(req->key, key)) {
            /* Poke the progress engine before exiting */
            status = poke_progress(key);
            HYDU_ERR_POP(status, "poke progress error\n");
            break;
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_get(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    int i, idx, found;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *key;
    const char *thrid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    found = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        pg = proxy->pg;
        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

        idx = -1;
        for (i = 0; i < pg->pg_process_count; i++)
            if (pg_scratch->ecount[i].fd == fd && pg_scratch->ecount[i].pid == pid) {
                idx = i;
                break;
            }

        HYDU_ASSERT(idx != -1, status);

        for (i = 0; i < pg->pg_process_count; i++) {
            if (pg_scratch->ecount[i].epoch < pg_scratch->ecount[idx].epoch) {
                /* We haven't reached a barrier yet; queue up request */
                status = HYD_pmcd_pmi_v2_queue_req(fd, pid, pgid, pmi, key, &pending_reqs);
                HYDU_ERR_POP(status, "unable to queue request\n");

                /* We are done */
                goto fn_exit;
            }
        }
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "kvs-get-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    if (found) {
        PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
        PMIU_cmd_add_str(&pmi_response, "value", run->val);
    } else {
        PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_fence(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    const char *thrid;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    int cur_epoch = -1;
    /* Try to find the epoch point of this process */
    for (i = 0; i < proxy->pg->pg_process_count; i++)
        if (pg_scratch->ecount[i].fd == fd && pg_scratch->ecount[i].pid == pid) {
            pg_scratch->ecount[i].epoch++;
            cur_epoch = pg_scratch->ecount[i].epoch;
            break;
        }

    if (i == proxy->pg->pg_process_count) {
        /* couldn't find the current process; find a NULL entry */

        for (i = 0; i < proxy->pg->pg_process_count; i++)
            if (pg_scratch->ecount[i].fd == HYD_FD_UNSET)
                break;

        pg_scratch->ecount[i].fd = fd;
        pg_scratch->ecount[i].pid = pid;
        pg_scratch->ecount[i].epoch = 0;
        cur_epoch = pg_scratch->ecount[i].epoch;
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "kvs-fence-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

    if (cur_epoch == pg_scratch->epoch) {
        pg_scratch->fence_count++;
        if (pg_scratch->fence_count % proxy->pg->pg_process_count == 0) {
            /* Poke the progress engine before exiting */
            status = poke_progress(NULL);
            HYDU_ERR_POP(status, "poke progress error\n");
            /* reset fence_count */
            pg_scratch->epoch++;
            pg_scratch->fence_count = 0;
            for (i = 0; i < proxy->pg->pg_process_count; i++) {
                if (pg_scratch->ecount[i].epoch >= pg_scratch->epoch) {
                    pg_scratch->fence_count++;
                }
            }
        }
    }

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

    const char *thrid, *val;
    char key[PMI_MAXKEYLEN];
    int maxprocs, preputcount, infokeycount, ret;
    int ncmds;
    char *execname, *path = NULL;

    int i, j, k, new_pgid;
    int argcnt;
    struct HYD_string_stash proxy_stash;
    char *control_port;
    struct HYD_string_stash stash;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    proxy_stash.strlist = NULL;

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");


    /* Here's the order of things we do:
     *
     *   1. Allocate a process group for the new set of spawned
     *      processes
     *
     *   2. Get all the common keys and deal with them
     *
     *   3. Create an executable list and parse the arguments from
     *      each segment.
     *
     *   5. Create a proxy list using the created executable list and
     *      spawn it.
     */

    val = PMIU_cmd_find_keyval(pmi, "ncmds");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR, "unable to find token: ncmds\n");
    ncmds = atoi(val);

    /* Allocate a new process group */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);
    new_pgid = pg->pgid + 1;

    status = HYDU_alloc_pg(&pg->next, new_pgid);
    HYDU_ERR_POP(status, "unable to allocate process group\n");

    pg = pg->next;

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg->spawner_pg = proxy->pg;

    for (j = 0; j < ncmds; j++) {
        /* For each segment, we create an exec structure */
        val = PMIU_cmd_find_keyval_segment(pmi, "maxprocs", "subcmd", j);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: maxprocs\n");
        maxprocs = atoi(val);
        pg->pg_process_count += maxprocs;

        val = PMIU_cmd_find_keyval_segment(pmi, "argc", "subcmd", j);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: argc\n");
        argcnt = atoi(val);

        val = PMIU_cmd_find_keyval_segment(pmi, "infokeycount", "subcmd", j);
        if (val)
            infokeycount = atoi(val);
        else
            infokeycount = 0;

        if (exec_list == NULL) {
            status = HYDU_alloc_exec(&exec_list);
            HYDU_ERR_POP(status, "unable to allocate exec\n");
            exec_list->appnum = 0;
            exec = exec_list;
        } else {
            for (exec = exec_list; exec->next; exec = exec->next);
            status = HYDU_alloc_exec(&exec->next);
            HYDU_ERR_POP(status, "unable to allocate exec\n");
            exec->next->appnum = exec->appnum + 1;
            exec = exec->next;
        }

        /* Info keys */
        for (i = 0; i < infokeycount; i++) {
            const char *info_key;

            MPL_snprintf(key, PMI_MAXKEYLEN, "infokey%d", i);
            val = PMIU_cmd_find_keyval_segment(pmi, key, "subcmd", j);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_key = val;

            MPL_snprintf(key, PMI_MAXKEYLEN, "infoval%d", i);
            val = PMIU_cmd_find_keyval_segment(pmi, key, "subcmd", j);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            if (!strcmp(info_key, "path")) {
                path = MPL_strdup(val);
            } else if (!strcmp(info_key, "wdir")) {
                exec->wdir = MPL_strdup(val);
            } else if (!strcmp(info_key, "host") || !strcmp(info_key, "hosts")) {
                char *info_val = MPL_strdup(val);
                char *saveptr;
                char *host = strtok_r(info_val, ",", &saveptr);
                while (host) {
                    status = HYDU_process_mfile_token(host, 1, &pg->user_node_list);
                    HYDU_ERR_POP(status, "error creating node list\n");
                    host = strtok_r(NULL, ",", &saveptr);
                }
                MPL_free(info_val);
            } else if (!strcmp(info_key, "hostfile")) {
                char *info_val = MPL_strdup(val);
                pg->user_node_list = NULL;
                status = HYDU_parse_hostfile(info_val, &pg->user_node_list,
                                             HYDU_process_mfile_token);
                HYDU_ERR_POP(status, "error parsing hostfile\n");
                MPL_free(info_val);
            } else {
                /* Unrecognized info key; ignore */
            }
        }

        status = HYDU_correct_wdir(&exec->wdir);
        HYDU_ERR_POP(status, "unable to correct wdir\n");

        val = PMIU_cmd_find_keyval_segment(pmi, "subcmd", "subcmd", j);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: subcmd\n");
        if (path == NULL)
            execname = MPL_strdup(val);
        else {
            HYD_STRING_STASH_INIT(stash);
            HYD_STRING_STASH(stash, MPL_strdup(path), status);
            HYD_STRING_STASH(stash, MPL_strdup("/"), status);
            HYD_STRING_STASH(stash, MPL_strdup(val), status);

            HYD_STRING_SPIT(stash, execname, status);
        }

        i = 0;
        exec->exec[i++] = execname;
        for (k = 0; k < argcnt; k++) {
            MPL_snprintf(key, PMI_MAXKEYLEN, "argv%d", k);
            val = PMIU_cmd_find_keyval_segment(pmi, key, "subcmd", j);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            exec->exec[i++] = MPL_strdup(val);
        }
        exec->exec[i++] = NULL;

        exec->proc_count = maxprocs;

        /* It is not clear what kind of environment needs to get
         * passed to the spawned process. Don't set anything here, and
         * let the proxy do whatever it does by default. */
        exec->env_prop = NULL;

        status = HYDU_env_create(&env, "PMI_SPAWNED", "1");
        HYDU_ERR_POP(status, "unable to create PMI_SPAWNED environment\n");
        exec->user_env = env;
    }

    status = HYD_pmcd_pmi_alloc_pg_scratch(pg);
    HYDU_ERR_POP(status, "unable to allocate pg scratch space\n");

    pg->pg_process_count = 0;
    for (exec = exec_list; exec; exec = exec->next)
        pg->pg_process_count += exec->proc_count;

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* Get the common keys and deal with them */
    val = PMIU_cmd_find_keyval(pmi, "preputcount");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: preputcount\n");
    preputcount = atoi(val);

    for (i = 0; i < preputcount; i++) {
        const char *preput_key, *preput_val;

        MPL_snprintf(key, PMI_MAXKEYLEN, "ppkey%d", i);
        val = PMIU_cmd_find_keyval(pmi, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_key = val;

        MPL_snprintf(key, PMI_MAXKEYLEN, "ppval%d", i);
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
        PMIU_cmd_init(&pmi_response, 2, "spawn-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "jobid", pg_scratch->kvs->kvsname);
        PMIU_cmd_add_str(&pmi_response, "nerrs", "0");;

        status = cmd_response(fd, pid, &pmi_response);
        HYDU_ERR_POP(status, "send command failed\n");
    }

  fn_exit:
    HYD_STRING_STASH_FREE(proxy_stash);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_publish(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *thrid, *val;
    char *name = NULL, *port = NULL;
    int success;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    if ((val = PMIU_cmd_find_keyval(pmi, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");
    name = MPL_strdup(val);
    HYDU_ERR_CHKANDJUMP(status, NULL == name, HYD_INTERNAL_ERROR, "%s", "");

    if ((val = PMIU_cmd_find_keyval(pmi, "port")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: port\n");
    port = MPL_strdup(val);

    status = HYD_pmcd_pmi_publish(name, port, &success);
    HYDU_ERR_POP(status, "error publishing service\n");

    struct PMIU_cmd pmi_response;
    char tmp[100];
    PMIU_cmd_init(&pmi_response, 2, "name-publish-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    if (!success) {
        MPL_snprintf(tmp, 100, "duplicate_service_%s", name);

        PMIU_cmd_add_str(&pmi_response, "rc", "1");
        PMIU_cmd_add_str(&pmi_response, "errmsg", tmp);
    } else {
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    MPL_free(name);
    MPL_free(port);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_unpublish(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *thrid, *name;
    int success;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    if ((name = PMIU_cmd_find_keyval(pmi, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");

    status = HYD_pmcd_pmi_unpublish(name, &success);
    HYDU_ERR_POP(status, "error unpublishing service\n");

    struct PMIU_cmd pmi_response;
    char tmp[100];
    PMIU_cmd_init(&pmi_response, 2, "name-unpublish-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    if (success) {
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else {
        MPL_snprintf(tmp, 100, "service_%s_not_found", name);

        PMIU_cmd_add_str(&pmi_response, "rc", "1");
        PMIU_cmd_add_str(&pmi_response, "errmsg", tmp);
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_lookup(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    const char *thrid, *name, *value;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    if ((name = PMIU_cmd_find_keyval(pmi, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");

    status = HYD_pmcd_pmi_lookup(name, &value);
    HYDU_ERR_POP(status, "error while looking up service\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "name-lookup-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    if (value) {
        PMIU_cmd_add_str(&pmi_response, "port", value);
        PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else {
        PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
        PMIU_cmd_add_str(&pmi_response, "rc", "1");
    }

    status = cmd_response(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* TODO: abort, create_kvs, destroy_kvs, getbyidx */
static struct HYD_pmcd_pmi_handle pmi_v2_handle_fns_foo[] = {
    {"info-getjobattr", fn_info_getjobattr},
    {"kvs-put", fn_kvs_put},
    {"kvs-get", fn_kvs_get},
    {"kvs-fence", fn_kvs_fence},
    {"spawn", fn_spawn},
    {"name-publish", fn_name_publish},
    {"name-unpublish", fn_name_unpublish},
    {"name-lookup", fn_name_lookup},
    {"\0", NULL}
};

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v2 = pmi_v2_handle_fns_foo;
