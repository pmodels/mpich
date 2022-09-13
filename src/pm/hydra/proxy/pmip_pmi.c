/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmip_pmi.h"
#include "pmip.h"
#include "bsci.h"
#include "demux.h"
#include "topo.h"
#include "uthash.h"
#include "pmi_v2_common.h"

#define debug(...)                              \
    {                                           \
        if (HYD_pmcd_pmip.user_global.debug)    \
            HYDU_dump(stdout, __VA_ARGS__);     \
    }

/* The number of key values we store has to be one less than the
 * number of PMI arguments allowed.  This is because, when we flush
 * the PMI keyvals upstream, the server will treat it as a single PMI
 * command.  We leave room for one extra argument, cmd=put, that needs
 * to be appended when flushing. */
#define CACHE_PUT_KEYVAL_MAXLEN  (MAX_PMI_ARGS - 1)

struct cache_put_elem {
    struct PMIU_token tokens[CACHE_PUT_KEYVAL_MAXLEN + 1];
    int keyval_len;
};

struct cache_elem {
    char *key;
    char *val;
    UT_hash_handle hh;
};

static struct cache_put_elem cache_put;
static struct cache_elem *cache_get = NULL, *hash_get = NULL;
static int num_elems = 0;

static void internal_init(void)
{
    /* all internal globals are statically initialized to 0 */
}

static void internal_finalize(void)
{
    HASH_CLEAR(hh, hash_get);
    for (int i = 0; i < num_elems; i++) {
        MPL_free((cache_get + i)->key);
        MPL_free((cache_get + i)->val);
    }
    MPL_free(cache_get);
}

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status poke_progress(const char *key)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *list_head = NULL, *list_tail = NULL;
    HYD_status status = HYD_SUCCESS;

    int count = 0;
    int has_key = false;
    for (req = pending_reqs; req; req = req->next) {
        if (strcmp(req->key, key) == 0) {
            has_key = true;
        }
        count++;
    }

    if (!has_key) {
        goto fn_exit;
    }

    for (int i = 0; i < count; i++) {
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
            status = fn_info_getnodeattr(req->fd, req->pmi);
            HYDU_ERR_POP(status, "getnodeattr returned error\n");

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

static int get_appnum(int local_rank)
{
    int ranks = 0;
    struct HYD_exec *exec;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {
        ranks += exec->proc_count;
        if (local_rank < ranks) {
            return exec->appnum;
        }
    }
    return -1;
}

static HYD_status send_cmd_upstream(struct PMIU_cmd *pmi, int fd)
{
    struct HYD_pmcd_hdr hdr;
    hdr.cmd = CMD_PMI;
    hdr.u.pmi.pid = fd;
    return HYD_pmcd_pmi_send(HYD_pmcd_pmip.upstream.control, pmi, &hdr,
                             HYD_pmcd_pmip.user_global.debug);
}

static HYD_status send_cmd_downstream(int fd, struct PMIU_cmd *pmi)
{
    return HYD_pmcd_pmi_send(fd, pmi, NULL, HYD_pmcd_pmip.user_global.debug);
}

/* "put" adds to cache_put locally. It gets flushed when there are more than
 * CACHE_PUT_KEYVAL_MAXLEN keyvals or when a barrier is received.
 */
static HYD_status cache_put_flush(int fd)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    if (cache_put.keyval_len == 0)
        goto fn_exit;

    debug("flushing %d put command(s) out\n", cache_put.keyval_len);

    struct PMIU_cmd pmi;
    PMIU_cmd_init_static(&pmi, 1, "put");
    HYDU_ASSERT(pmi.num_tokens < MAX_PMI_ARGS, status);
    for (int i = 0; i < cache_put.keyval_len; i++) {
        PMIU_cmd_add_str(&pmi, cache_put.tokens[i].key, cache_put.tokens[i].val);
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "forwarding command upstream:\n");
        HYD_pmcd_pmi_dump(&pmi);
    }
    status = send_cmd_upstream(&pmi, fd);
    HYDU_ERR_POP(status, "error sending command upstream\n");

    for (int i = 0; i < cache_put.keyval_len; i++) {
        MPL_free((void *) cache_put.tokens[i].key);
        MPL_free((void *) cache_put.tokens[i].val);
    }
    cache_put.keyval_len = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_init(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    int pmi_version, pmi_subversion;
    HYD_PMI_GET_INTVAL(pmi, "pmi_version", pmi_version);
    HYD_PMI_GET_INTVAL(pmi, "pmi_subversion", pmi_subversion);

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "response_to_init");
    if (pmi_version == 1 && pmi_subversion <= 1) {
        PMIU_cmd_add_str(&pmi_response, "pmi_version", "1");
        PMIU_cmd_add_str(&pmi_response, "pmi_subversion", "1");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else if (pmi_version == 2 && pmi_subversion == 0) {
        PMIU_cmd_add_str(&pmi_response, "pmi_version", "2");
        PMIU_cmd_add_str(&pmi_response, "pmi_subversion", "0");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else {    /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);
    }

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    static int global_init = 1;
    if (global_init) {
        internal_init();
        global_init = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_fullinit(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    int id;
    if (pmi->version == 1) {
        /* initack */
        HYD_PMI_GET_INTVAL(pmi, "pmiid", id);
    } else {
        /* fullinit */
        HYD_PMI_GET_INTVAL(pmi, "pmirank", id);
    }

    /* Store the PMI_ID to fd mapping */
    int local_rank = -1;
    for (int i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pmi_rank[i] == id) {
            HYD_pmcd_pmip.downstream.pmi_fd[i] = fd;
            HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 1;
            local_rank = i;
            break;
        }
    }
    HYDU_ASSERT(local_rank != -1, status);

    int size = HYD_pmcd_pmip.system_global.global_process_count;
    int rank = id;
    int debug = HYD_pmcd_pmip.user_global.debug;
    int appnum = get_appnum(local_rank);

    struct PMIU_cmd pmi_response;
    if (pmi->version == 1) {
        PMIU_cmd_init_static(&pmi_response, 1, "initack");
        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");

        PMIU_cmd_init_static(&pmi_response, pmi->version, "set");
        PMIU_cmd_add_int(&pmi_response, "size", size);
        PMIU_cmd_add_int(&pmi_response, "rank", rank);
        PMIU_cmd_add_int(&pmi_response, "debug", debug);

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    } else {
        PMIU_cmd_init_static(&pmi_response, 2, "fullinit-response");
        PMIU_cmd_add_str(&pmi_response, "pmi-version", "2");
        PMIU_cmd_add_str(&pmi_response, "pmi-subversion", "0");
        PMIU_cmd_add_int(&pmi_response, "rank", id);
        PMIU_cmd_add_int(&pmi_response, "size", size);;
        PMIU_cmd_add_int(&pmi_response, "appnum", appnum);
        if (HYD_pmcd_pmip.local.spawner_kvsname) {
            PMIU_cmd_add_str(&pmi_response, "spawner-jobid", HYD_pmcd_pmip.local.spawner_kvsname);
        }
        PMIU_cmd_add_str(&pmi_response, "rc", "0");

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_maxes(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "maxes");
    PMIU_cmd_add_int(&pmi_response, "kvsname_max", PMI_MAXKVSLEN);
    PMIU_cmd_add_int(&pmi_response, "keylen_max", PMI_MAXKEYLEN);
    PMIU_cmd_add_int(&pmi_response, "vallen_max", PMI_MAXVALLEN);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_appnum(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    /* Get the process index */
    int idx = -1;
    for (int i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pmi_fd[i] == fd) {
            idx = i;
            break;
        }
    }
    HYDU_ASSERT(idx != -1, status);

    int appnum;
    appnum = get_appnum(idx);

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "appnum");
    PMIU_cmd_add_int(&pmi_response, "appnum", appnum);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_my_kvsname(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "my_kvsname");
    PMIU_cmd_add_str(&pmi_response, "kvsname", HYD_pmcd_pmip.local.kvs->kvsname);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_usize(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int universe_size;
    if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_SYSTEM) {
        universe_size = HYD_pmcd_pmip.system_global.global_core_map.global_count;
    } else if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_INFINITE) {
        universe_size = -1;
    } else {
        universe_size = HYD_pmcd_pmip.user_global.usize;
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "universe_size");
    PMIU_cmd_add_int(&pmi_response, "size", universe_size);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pmi->version == 2) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI-v2 doesn't support %s\n", pmi->cmd);
    }

    const char *key;
    HYD_PMI_GET_STRVAL(pmi, "key", key);

    bool found = false;
    const char *val;
    if (strcmp(key, "PMI_process_mapping") == 0) {
        found = true;
        val = HYD_pmcd_pmip.system_global.pmi_process_mapping;
    } else if (!strcmp(key, "PMI_hwloc_xmlfile")) {
        val = HYD_pmip_get_hwloc_xmlfile();
        if (val) {
            found = true;
        }
    } else {
        struct cache_elem *elem = NULL;
        HASH_FIND_STR(hash_get, key, elem);
        if (elem) {
            found = true;
            val = elem->val;
        }
    }

    if (found) {
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, pmi->version, "get_result");

        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
        PMIU_cmd_add_str(&pmi_response, "value", val);

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    } else {
        /* if we can't find the key locally, ask upstream */
        status = send_cmd_upstream(pmi, fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_put(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pmi->version == 2) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI-v2 doesn't support %s\n", pmi->cmd);
    }

    const char *key, *val;
    HYD_PMI_GET_STRVAL(pmi, "key", key);
    HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "value", val, NULL);

    /* add to the cache */
    int i = cache_put.keyval_len++;
    cache_put.tokens[i].key = MPL_strdup(key);
    if (val) {
        cache_put.tokens[i].val = MPL_strdup(val);
    } else {
        cache_put.tokens[i].val = NULL;
    }
    debug("cached command: %s=%s\n", key, val);

    if (cache_put.keyval_len >= CACHE_PUT_KEYVAL_MAXLEN) {
        cache_put_flush(fd);
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, 1, "put_result");
    PMIU_cmd_add_str(&pmi_response, "rc", "0");
    PMIU_cmd_add_str(&pmi_response, "msg", "success");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_keyval_cache(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: leak of abstraction of the pmi object */

    /* allocate a larger space for the cached keyvals, copy over the
     * older keyvals and add the new ones in */
    HASH_CLEAR(hh, hash_get);
    HYDU_REALLOC_OR_JUMP(cache_get, struct cache_elem *,
                         (sizeof(struct cache_elem) * (num_elems + pmi->num_tokens)), status);

    int i;
    for (i = 0; i < num_elems; i++) {
        struct cache_elem *elem = cache_get + i;
        HASH_ADD_STR(hash_get, key, elem, MPL_MEM_PM);
    }
    for (; i < num_elems + pmi->num_tokens; i++) {
        struct cache_elem *elem = cache_get + i;
        elem->key = MPL_strdup(pmi->tokens[i - num_elems].key);
        HYDU_ERR_CHKANDJUMP(status, NULL == elem->key, HYD_INTERNAL_ERROR, "%s", "");
        elem->val = MPL_strdup(pmi->tokens[i - num_elems].val);
        HYDU_ERR_CHKANDJUMP(status, NULL == elem->val, HYD_INTERNAL_ERROR, "%s", "");
        HASH_ADD_STR(hash_get, key, elem, MPL_MEM_PM);
    }
    num_elems += pmi->num_tokens;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_barrier_in(int fd, struct PMIU_cmd *pmi)
{
    static int barrier_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    barrier_count++;
    if (barrier_count == HYD_pmcd_pmip.local.proxy_process_count) {
        barrier_count = 0;

        cache_put_flush(fd);

        status = send_cmd_upstream(pmi, fd);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_barrier_out(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "barrier_out");

    for (int i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        status = send_cmd_downstream(HYD_pmcd_pmip.downstream.pmi_fd[i], &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_finalize(int fd, struct PMIU_cmd *pmi)
{
    static int finalize_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    if (pmi->version == 1) {
        PMIU_cmd_init_static(&pmi_response, 1, "finalize_ack");
    } else {
        const char *thrid;
        thrid = PMIU_cmd_find_keyval(pmi, "thrid");

        PMIU_cmd_init_static(&pmi_response, 2, "finalize-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    }

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

    finalize_count++;

    if (finalize_count == HYD_pmcd_pmip.local.proxy_process_count) {
        /* All processes have finalized */
        internal_finalize();
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_job_getid(int fd, struct PMIU_cmd *pmi)
{
    const char *thrid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (pmi->version == 1) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI-v1 doesn't support %s\n", pmi->cmd);
    }

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, 2, "job-getid-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_str(&pmi_response, "jobid", HYD_pmcd_pmip.local.kvs->kvsname);
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_info_putnodeattr(int fd, struct PMIU_cmd *pmi)
{
    int ret;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    const char *key, *val, *thrid;
    HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "thrid", thrid, NULL);
    HYD_PMI_GET_STRVAL(pmi, "key", key);
    HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "value", val, NULL);

    status = HYD_pmcd_pmi_add_kvs(key, val, HYD_pmcd_pmip.local.kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "info-putnodeattr-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

    status = poke_progress(key);
    HYDU_ERR_POP(status, "poke progress error\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_info_getnodeattr(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    const char *key, *waitval, *thrid;
    HYD_PMI_GET_STRVAL(pmi, "key", key);
    HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "wait", waitval, NULL);
    HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "thrid", thrid, NULL);

    /* if a predefined value is not found, we let the code fall back
     * to regular search and return an error to the client */

    int found;
    found = 0;

    struct HYD_pmcd_pmi_kvs_pair *run;
    for (run = HYD_pmcd_pmip.local.kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (!found && waitval && strcmp(waitval, "TRUE") == 0) {
        status = HYD_pmcd_pmi_v2_queue_req(fd, -1, -1, pmi, key, &pending_reqs);
        HYDU_ERR_POP(status, "unable to queue request\n");
        goto fn_exit;
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "info-getnodeattr-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    if (found) {        /* We found the attribute */
        PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
        PMIU_cmd_add_str(&pmi_response, "value", run->val);
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else {
        PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    }

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_info_getjobattr(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    const char *key, *thrid;
    HYD_PMI_GET_STRVAL(pmi, "key", key);
    HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "thrid", thrid, NULL);

    if (!strcmp(key, "PMI_process_mapping")) {
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, pmi->version, "info-getjobattr-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
        PMIU_cmd_add_str(&pmi_response, "value", HYD_pmcd_pmip.system_global.pmi_process_mapping);
        PMIU_cmd_add_str(&pmi_response, "rc", "0");

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");
    } else if (!strcmp(key, "PMI_hwloc_xmlfile")) {
        const char *xmlfile = HYD_pmip_get_hwloc_xmlfile();

        struct PMIU_cmd pmi_response;
        PMIU_cmd_init_static(&pmi_response, 2, "info-getjobattr-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        if (xmlfile) {
            PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
            PMIU_cmd_add_str(&pmi_response, "value", xmlfile);
        } else {
            PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
        }
        PMIU_cmd_add_str(&pmi_response, "rc", "0");

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");
    } else {
        status = send_cmd_upstream(pmi, fd);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
