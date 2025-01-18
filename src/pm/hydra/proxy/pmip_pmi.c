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
static bool is_static = false;

/* info_getnodeattr will wait for info_putnodeattr */
struct HYD_pmcd_pmi_v2_reqs {
    struct pmip_downstream *downstream_ptr;
    struct PMIU_cmd *pmi;
    const char *key;

    struct HYD_pmcd_pmi_v2_reqs *prev;
    struct HYD_pmcd_pmi_v2_reqs *next;
};

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status HYD_pmcd_pmi_v2_queue_req(struct pmip_downstream *p, struct PMIU_cmd *pmi,
                                            const char *key)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC_OR_JUMP(req, struct HYD_pmcd_pmi_v2_reqs *, sizeof(struct HYD_pmcd_pmi_v2_reqs),
                        status);
    req->downstream_ptr = p;
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
            status = fn_info_getnodeattr(req->downstream_ptr, req->pmi);
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

static HYD_status send_cmd_upstream(struct pmip_pg *pg, struct PMIU_cmd *pmi, int process_fd)
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
    hdr.u.pmi.process_fd = process_fd;
    status = PMIP_send_hdr_upstream(pg, &hdr, buf, buflen);
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
static HYD_status cache_put_flush(struct pmip_pg *pg)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    if (utarray_len(pg->kvs_batch) == 0)
        goto fn_exit;

    debug("flushing %d put command(s) out\n", utarray_len(pg->kvs_batch));

    struct PMIU_cmd pmi;
    PMIU_msg_set_query(&pmi, PMIU_WIRE_V1, PMIU_CMD_MPUT, false /* not static */);
    HYDU_ASSERT(pmi.num_tokens < MAX_PMI_ARGS, status);

    const char **p = (const char **) utarray_front(pg->kvs_batch);
    while (p) {
        struct pmip_kvs *s;
        HASH_FIND_STR(pg->kvs, *p, s);
        HYDU_ASSERT(s, status);
        PMIU_cmd_add_str(&pmi, s->key, s->val);

        p = (const char **) utarray_next(pg->kvs_batch, p);
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "forwarding command upstream:\n");
        HYD_pmcd_pmi_dump(&pmi);
    }
    status = send_cmd_upstream(pg, &pmi, 0 /* dummy fd */);
    HYDU_ERR_POP(status, "error sending command upstream\n");

    utarray_clear(pg->kvs_batch);
    PMIU_cmd_free_buf(&pmi);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_init(struct pmip_downstream *p, struct PMIU_cmd *pmi)
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

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_fullinit(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    int id;
    pmi_errno = PMIU_msg_get_query_fullinit(pmi, &id);
    HYDU_ASSERT(!pmi_errno, status);

    struct pmip_pg *pg;
    pg = PMIP_pg_from_downstream(p);
    HYDU_ASSERT(pg, status);

    if (p->idx == -1) {
        /* the input p is a pseudo downstream,
         * find and initialize the real downstream
         */
        struct pmip_downstream *tmp = NULL;
        for (int i = 0; i < pg->num_procs; i++) {
            if (pg->downstreams[i].pmi_rank == id) {
                tmp = &pg->downstreams[i];
                tmp->pmi_fd = p->pmi_fd;
                tmp->pmi_fd_active = 1;
                break;
            }
        }
        HYDU_ASSERT(tmp, status);
        p = tmp;
    }

    int size = pg->global_process_count;
    int rank = id;
    int debug = HYD_pmcd_pmip.user_global.debug;
    int appnum = p->pmi_appnum;

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_fullinit(pmi, &pmi_response, is_static, rank, size, appnum,
                                               pg->spawner_kvsname, debug);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_maxes(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_maxes(pmi, &pmi_response, is_static,
                                            PMI_MAXKVSLEN, PMI_MAXKEYLEN, PMI_MAXVALLEN);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_appnum(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    int appnum;
    appnum = p->pmi_appnum;

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_appnum(pmi, &pmi_response, is_static, appnum);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_get_my_kvsname(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_kvsname(pmi, &pmi_response, is_static,
                                              PMIP_pg_from_downstream(p)->kvsname);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static int get_universe_size(struct pmip_downstream *p)
{
    int universe_size;
    if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_SYSTEM) {
        universe_size = PMIP_pg_from_downstream(p)->global_core_map.global_count;
    } else if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_INFINITE) {
        universe_size = -1;
    } else {
        universe_size = HYD_pmcd_pmip.user_global.usize;
    }
    return universe_size;
}

HYD_status fn_get_usize(struct pmip_downstream * p, struct PMIU_cmd * pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    int universe_size = get_universe_size(p);

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response_universe(pmi, &pmi_response, is_static, universe_size);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static const char *get_jobattr(struct pmip_downstream *p, const char *key)
{
    if (strcmp(key, "PMI_process_mapping") == 0) {
        return PMIP_pg_from_downstream(p)->pmi_process_mapping;
    } else if (!strcmp(key, "PMI_hwloc_xmlfile")) {
        return HYD_pmip_get_hwloc_xmlfile();
    } else if (!strcmp(key, "universeSize")) {
        static char universe_str[64];
        int universe_size = get_universe_size(p);
        snprintf(universe_str, 64, "%d", universe_size);
        return universe_str;
    } else if (!strcmp(key, "PMI_mpi_memory_alloc_kinds")) {
        return HYD_pmcd_pmip.user_global.memory_alloc_kinds;
    }
    return NULL;
}

HYD_status fn_get(struct pmip_downstream * p, struct PMIU_cmd * pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    const char *kvsname;        /* unused */
    const char *key;
    PMIU_msg_get_query_get(pmi, &kvsname, &key);

    bool found = false;
    const char *val;
    if (strncmp(key, "PMI_", 4) == 0 || strcmp(key, "universeSize") == 0) {
        val = get_jobattr(p, key);
        if (val) {
            found = true;
        }
    } else {
        struct pmip_kvs *s = NULL;
        HASH_FIND_STR(PMIP_pg_from_downstream(p)->kvs, key, s);
        if (s) {
            found = true;
            val = s->val;
        }
    }

    if (found) {
        struct PMIU_cmd pmi_response;
        PMIU_msg_set_response_get(pmi, &pmi_response, is_static, found, val);

        status = send_cmd_downstream(p->pmi_fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");

        PMIU_cmd_free_buf(&pmi_response);
    } else {
        /* if we can't find the key locally, ask upstream */
        status = send_cmd_upstream(PMIP_pg_from_downstream(p), pmi, p->pmi_fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_put(struct pmip_downstream *p, struct PMIU_cmd *pmi)
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

        status = send_cmd_downstream(p->pmi_fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");

        PMIU_cmd_free_buf(&pmi_response);

        goto fn_exit;
    }

    struct pmip_pg *pg = PMIP_pg_from_downstream(p);

    struct pmip_kvs *s;
    HASH_FIND_STR(pg->kvs, key, s);
    if (s) {
        /* key exist, replace the value */
        MPL_free(s->val);
        s->val = MPL_strdup(val);
    } else {
        s = MPL_malloc(sizeof(*s), MPL_MEM_OTHER);
        s->key = MPL_strdup(key);
        s->val = MPL_strdup(val);

        HASH_ADD_KEYPTR(hh, pg->kvs, s->key, strlen(s->key), s, MPL_MEM_OTHER);
    }
    utarray_push_back(pg->kvs_batch, &(s->key), MPL_MEM_OTHER);

    debug("cached command: %s=%s\n", key, val);

    if (utarray_len(pg->kvs_batch) >= CACHE_PUT_KEYVAL_MAXLEN) {
        cache_put_flush(pg);
    }

    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

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

    for (int i = 0; i < num_tokens; i++) {
        struct pmip_kvs *s;
        HASH_FIND_STR(pg->kvs, tokens[i].key, s);
        if (s) {
            /* key exist, replace the value */
            MPL_free(s->val);
            s->val = MPL_strdup(tokens[i].val);
        } else {
            s = MPL_malloc(sizeof(*s), MPL_MEM_OTHER);
            s->key = MPL_strdup(tokens[i].key);
            s->val = MPL_strdup(tokens[i].val);
            HASH_ADD_KEYPTR(hh, pg->kvs, s->key, strlen(s->key), s, MPL_MEM_OTHER);
        }
    }

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status fn_barrier_in(struct pmip_downstream * p, struct PMIU_cmd * pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct pmip_pg *pg = PMIP_pg_from_downstream(p);
    pg->barrier_count++;
    if (pg->barrier_count == pg->num_procs) {
        pg->barrier_count = 0;

        cache_put_flush(pg);

        status = send_cmd_upstream(pg, pmi, p->pmi_fd);
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

HYD_status fn_finalize(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct pmip_pg *pg = PMIP_pg_from_downstream(p);

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    PMIU_cmd_free_buf(&pmi_response);

    if (HYD_pmcd_pmip.user_global.auto_cleanup) {
        /* deregister to prevent the cleanup kill on fd close */
        status = HYDT_dmx_deregister_fd(p->pmi_fd);
        HYDU_ERR_POP(status, "unable to deregister fd\n");
        close(p->pmi_fd);
    }

    /* mark singleton's stdio sockets as closed */
    if (pg->is_singleton) {
        HYDU_ASSERT(pg->num_procs == 1, status);
        p->out = HYD_FD_CLOSED;
        p->err = HYD_FD_CLOSED;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_info_putnodeattr(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    struct PMIU_cmd pmi_response;
    HYDU_FUNC_ENTER();

    const char *key, *val;
    pmi_errno = PMIU_msg_get_query_putnodeattr(pmi, &key, &val);
    HYDU_ASSERT(!pmi_errno, status);

    if (strncmp(key, "PMI_", 4) == 0) {
        status = PMIU_msg_set_response_fail(pmi, &pmi_response, is_static,
                                            1, "Keys with PMI_ prefix are reserved");
        HYDU_ASSERT(!pmi_errno, status);

        status = send_cmd_downstream(p->pmi_fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");

        PMIU_cmd_free_buf(&pmi_response);

        goto fn_exit;
    }

    struct pmip_kvs *s;
    s = MPL_malloc(sizeof(*s), MPL_MEM_OTHER);
    s->key = MPL_strdup(key);
    s->val = MPL_strdup(val);

    /* NOTE: uthash are all macros, very tricky to use:
     *   1. avoid function call in params.
     *   2. pg->kvs may change, thus cannot be local pointer variable.
     *   3. use s->key not key since uthash stores it by ptr.
     */
    struct pmip_pg *pg;
    pg = PMIP_pg_from_downstream(p);
    HASH_ADD_KEYPTR(hh, pg->kvs, s->key, strlen(s->key), s, MPL_MEM_OTHER);

    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

    PMIU_cmd_free_buf(&pmi_response);

    status = poke_progress(key);
    HYDU_ERR_POP(status, "poke progress error\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status fn_info_getnodeattr(struct pmip_downstream *p, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    struct pmip_kvs *kvs = PMIP_pg_from_downstream(p)->kvs;
    HYDU_FUNC_ENTER();

    const char *key;
    bool wait;
    pmi_errno = PMIU_msg_get_query_getnodeattr(pmi, &key, &wait);
    HYDU_ASSERT(!pmi_errno, status);

    /* if a predefined value is not found, we let the code fall back
     * to regular search and return an error to the client */

    struct pmip_kvs *s;
    const char *val;
    int found = 0;
    HASH_FIND_STR(kvs, key, s);
    if (s) {
        val = s->val;
        found = 1;
    }

    if (!found && wait) {
        status = HYD_pmcd_pmi_v2_queue_req(p, pmi, key);
        HYDU_ERR_POP(status, "unable to queue request\n");
        goto fn_exit;
    }

    struct PMIU_cmd pmi_response;
    if (found) {
        pmi_errno = PMIU_msg_set_response_getnodeattr(pmi, &pmi_response, is_static, true, val);
    } else {
        pmi_errno = PMIU_msg_set_response_fail(pmi, &pmi_response, is_static, 1, "not_found");
    }
    HYDU_ASSERT(!pmi_errno, status);

    status = send_cmd_downstream(p->pmi_fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

    PMIU_cmd_free_buf(&pmi_response);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
