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

#define debug(...)                              \
    {                                           \
        if (HYD_pmcd_pmip.user_global.debug)    \
            HYDU_dump(stdout, __VA_ARGS__);     \
    }

/* use static buffer in PMIU_cmd facility */
static bool is_static = true;

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

/* TODO: move static variables inside pmip_pg */
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

/* info_getnodeattr will wait for info_putnodeattr */
struct HYD_pmcd_pmi_v2_reqs {
    int fd;
    struct PMIU_cmd *pmi;
    const char *key;

    struct HYD_pmcd_pmi_v2_reqs *prev;
    struct HYD_pmcd_pmi_v2_reqs *next;
};

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status HYD_pmcd_pmi_v2_queue_req(int fd, struct PMIU_cmd *pmi, const char *key)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC_OR_JUMP(req, struct HYD_pmcd_pmi_v2_reqs *, sizeof(struct HYD_pmcd_pmi_v2_reqs),
                        status);
    req->fd = fd;
    req->prev = NULL;
    req->next = NULL;

    req->pmi = PMIU_cmd_dup(pmi);
    req->key = MPL_strdup(key);

    if (pending_reqs == NULL)
        pending_reqs = req;
    else {
        for (tmp = pending_reqs; tmp->next; tmp = tmp->next);
        tmp->next = req;
        req->prev = tmp;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

/* check pending kvs get when we get fn_info_putnodeattr */
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

static HYD_status send_cmd_upstream(struct PMIU_cmd *pmi, int fd)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    char *buf = NULL;
    int buflen = 0;

    int pmi_errno = PMIU_cmd_output(pmi, &buf, &buflen);
    HYDU_ASSERT(!pmi_errno, status);

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "Sending upstream internal PMI command:\n");
        if (buf[buflen - 1] == '\n') {
            HYDU_dump_noprefix(stdout, "    %s", buf);
        } else {
            HYDU_dump_noprefix(stdout, "%s\n", buf);
        }
    }

    struct HYD_pmcd_hdr hdr;
    hdr.cmd = CMD_PMI;
    hdr.u.pmi.pmi_version = pmi->version;
    hdr.u.pmi.process_fd = fd;
    status = PMIP_send_hdr_upstream(&hdr, buf, buflen);
    HYDU_ERR_POP(status, "unable to send hdr\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
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
    PMIU_msg_set_query(&pmi, PMIU_WIRE_V1, PMIU_CMD_MPUT, false /* not static */);
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
        MPL_free(cache_put.tokens[i].key);
        MPL_free(cache_put.tokens[i].val);
    }
    cache_put.keyval_len = 0;
    PMIU_cmd_free_buf(&pmi);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_init(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    int pmi_version, pmi_subversion;
    pmi_errno = PMIU_msg_get_query_init(pmi, &pmi_version, &pmi_subversion);
    HYDU_ASSERT(!pmi_errno, status);

    struct PMIU_cmd pmi_response;
    if (pmi_version == 1 && pmi_subversion <= 1) {
        pmi_errno = PMIU_msg_set_response_init(pmi, &pmi_response, is_static, 1, 1);
        HYDU_ASSERT(!pmi_errno, status);
    } else if (pmi_version == 2 && pmi_subversion == 0) {
        pmi_errno = PMIU_msg_set_response_init(pmi, &pmi_response, is_static, 2, 0);
        HYDU_ASSERT(!pmi_errno, status);
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
    int pmi_errno;
    HYDU_FUNC_ENTER();

    int id;
    pmi_errno = PMIU_msg_get_query_fullinit(pmi, &id);
    HYDU_ASSERT(!pmi_errno, status);

    struct pmip_pg *pg;
    pg = PMIP_pg_0();
    HYDU_ASSERT(pg, status);

    /* Store the PMI_ID to fd mapping */
    struct pmip_downstream *p;
    p = NULL;
    for (int i = 0; i < pg->num_procs; i++) {
        p = &pg->downstreams[i];
        if (p->pmi_rank == id) {
            p->pmi_fd = fd;
            p->pmi_fd_active = 1;
            break;
        }
    }
    HYDU_ASSERT(p, status);

    int size = HYD_pmcd_pmip.system_global.global_process_count;
    int rank = id;
    int debug = HYD_pmcd_pmip.user_global.debug;
    int appnum = p->pmi_appnum;

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_fullinit(pmi, &pmi_response, is_static, rank, size, appnum,
                                               HYD_pmcd_pmip.local.spawner_kvsname, debug);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_maxes(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_maxes(pmi, &pmi_response, is_static,
                                            PMI_MAXKVSLEN, PMI_MAXKEYLEN, PMI_MAXVALLEN);
    HYDU_ASSERT(!pmi_errno, status);

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
    int pmi_errno;
    HYDU_FUNC_ENTER();

    /* Get the process index */
    struct pmip_downstream *p = PMIP_find_downstream_by_fd(fd);
    HYDU_ASSERT(p, status);

    int appnum;
    appnum = p->pmi_appnum;

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_appnum(pmi, &pmi_response, is_static, appnum);
    HYDU_ASSERT(!pmi_errno, status);

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
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_kvsname(pmi, &pmi_response, is_static,
                                              HYD_pmcd_pmip.local.kvs->kvsname);
    HYDU_ASSERT(!pmi_errno, status);

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
    int pmi_errno;
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
    pmi_errno = PMIU_msg_set_response_universe(pmi, &pmi_response, is_static, universe_size);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static const char *get_jobattr(const char *key)
{
    if (strcmp(key, "PMI_process_mapping") == 0) {
        return HYD_pmcd_pmip.system_global.pmi_process_mapping;
    } else if (!strcmp(key, "PMI_hwloc_xmlfile")) {
        return HYD_pmip_get_hwloc_xmlfile();
    }
    return NULL;
}

HYD_status fn_get(int fd, struct PMIU_cmd * pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    const char *kvsname;        /* unused */
    const char *key;
    PMIU_msg_get_query_get(pmi, &kvsname, &key);

    bool found = false;
    const char *val;
    if (strncmp(key, "PMI_", 4) == 0) {
        val = get_jobattr(key);
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
        PMIU_msg_set_response_get(pmi, &pmi_response, is_static, val, found);

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
    int pmi_errno;
    struct PMIU_cmd pmi_response;
    HYDU_FUNC_ENTER();

    if (pmi->version == 2) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI-v2 doesn't support %s\n", pmi->cmd);
    }

    const char *kvsname, *key, *val;
    pmi_errno = PMIU_msg_get_query_put(pmi, &kvsname, &key, &val);
    HYDU_ASSERT(!pmi_errno, status);

    if (strncmp(key, "PMI_", 4) == 0) {
        status = PMIU_msg_set_response_fail(pmi, &pmi_response, is_static,
                                            1, "Keys with PMI_ prefix are reserved");
        HYDU_ASSERT(!pmi_errno, status);

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");

        goto fn_exit;
    }

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

    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_keyval_cache(struct pmip_pg *pg, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int num_tokens;
    const struct PMIU_token *tokens;
    PMIU_cmd_get_tokens(pmi, &num_tokens, &tokens);

    /* allocate a larger space for the cached keyvals, copy over the
     * older keyvals and add the new ones in */
    HASH_CLEAR(hh, hash_get);
    HYDU_REALLOC_OR_JUMP(cache_get, struct cache_elem *,
                         (sizeof(struct cache_elem) * (num_elems + num_tokens)), status);

    int i;
    for (i = 0; i < num_elems; i++) {
        struct cache_elem *elem = cache_get + i;
        HASH_ADD_STR(hash_get, key, elem, MPL_MEM_PM);
    }
    for (; i < num_elems + num_tokens; i++) {
        struct cache_elem *elem = cache_get + i;
        elem->key = MPL_strdup(tokens[i - num_elems].key);
        HYDU_ERR_CHKANDJUMP(status, NULL == elem->key, HYD_INTERNAL_ERROR, "%s", "");
        elem->val = MPL_strdup(tokens[i - num_elems].val);
        HYDU_ERR_CHKANDJUMP(status, NULL == elem->val, HYD_INTERNAL_ERROR, "%s", "");
        HASH_ADD_STR(hash_get, key, elem, MPL_MEM_PM);
    }
    num_elems += num_tokens;

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

    struct pmip_downstream *p = PMIP_find_downstream_by_fd(fd);

    barrier_count++;
    if (barrier_count == p->pg->num_procs) {
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

HYD_status fn_barrier_out(struct pmip_pg *pg, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init_static(&pmi_response, pmi->version, "barrier_out");

    for (int i = 0; i < pg->num_procs; i++) {
        status = send_cmd_downstream(pg->downstreams[i].pmi_fd, &pmi_response);
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
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct pmip_downstream *p = PMIP_find_downstream_by_fd(fd);
    struct pmip_pg *pg = p->pg;

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

    finalize_count++;

    /* mark singleton's stdio sockets as closed */
    if (HYD_pmcd_pmip.is_singleton) {
        HYDU_ASSERT(pg->num_procs == 1, status);
        p->out = HYD_FD_CLOSED;
        p->err = HYD_FD_CLOSED;
    }

    if (finalize_count == pg->num_procs) {
        /* All processes have finalized */
        internal_finalize();
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_info_putnodeattr(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int ret, pmi_errno;
    struct PMIU_cmd pmi_response;
    HYDU_FUNC_ENTER();

    const char *key, *val;
    pmi_errno = PMIU_msg_get_query_putnodeattr(pmi, &key, &val);
    HYDU_ASSERT(!pmi_errno, status);

    if (strncmp(key, "PMI_", 4) == 0) {
        status = PMIU_msg_set_response_fail(pmi, &pmi_response, is_static,
                                            1, "Keys with PMI_ prefix are reserved");
        HYDU_ASSERT(!pmi_errno, status);

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");

        goto fn_exit;
    }

    status = HYD_pmcd_pmi_add_kvs(key, val, HYD_pmcd_pmip.local.kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

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
    int pmi_errno;
    HYDU_FUNC_ENTER();

    const char *key;
    bool wait;
    pmi_errno = PMIU_msg_get_query_getnodeattr(pmi, &key, &wait);
    HYDU_ASSERT(!pmi_errno, status);

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

    if (!found && wait) {
        status = HYD_pmcd_pmi_v2_queue_req(fd, pmi, key);
        HYDU_ERR_POP(status, "unable to queue request\n");
        goto fn_exit;
    }

    struct PMIU_cmd pmi_response;
    if (found) {
        pmi_errno =
            PMIU_msg_set_response_getnodeattr(pmi, &pmi_response, is_static, run->val, true);
    } else {
        pmi_errno = PMIU_msg_set_response_fail(pmi, &pmi_response, is_static, 1, "not_found");
    }
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
