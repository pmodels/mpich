/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

struct HYD_pmcd_pmi_v2_reqs {
    struct HYD_proxy *proxy;
    int process_fd;
    int pgid;
    struct PMIU_cmd *pmi;
    const char *key;

    struct HYD_pmcd_pmi_v2_reqs *prev;
    struct HYD_pmcd_pmi_v2_reqs *next;
};

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static bool check_epoch_reached(struct HYD_pg *pg, int process_fd);
static HYD_status HYD_pmcd_pmi_v2_queue_req(struct HYD_proxy *proxy, int process_fd, int pgid,
                                            struct PMIU_cmd *pmi, const char *key);
static HYD_status check_pending_reqs(const char *key);

static const bool is_static = true;

HYD_status HYD_pmiserv_kvs_get(struct HYD_proxy *proxy, int process_fd, int pgid,
                               struct PMIU_cmd *pmi, bool sync)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg = PMISERV_pg_by_id(proxy->pgid);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    const char *kvsname;
    const char *key;
    pmi_errno = PMIU_msg_get_query_get(pmi, &kvsname, &key);
    if (kvsname && strcmp(pg_scratch->kvsname, kvsname) != 0) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "kvsname (%s) does not match this group's kvs space (%s)\n",
                            kvsname, pg_scratch->kvsname);
    }

    int found;
    const char *val;
    found = 0;
    val = NULL;
    if (strcmp(key, "PMI_dead_processes") == 0) {
        found = 1;
        val = pg_scratch->dead_processes;
    } else {
        HYD_kvs_find(pg_scratch->kvs, key, &val, &found);
    }

    if (!found && sync) {
        /* check whether all proxies have arrived at the same epoch or enqueue
         * the "get". A "put" (from another proxy) will poke the progress.
         */
        if (!check_epoch_reached(pg, process_fd)) {
            status = HYD_pmcd_pmi_v2_queue_req(proxy, process_fd, pgid, pmi, key);
            HYDU_ERR_POP(status, "unable to queue request\n");
            goto fn_exit;
        }
    }

    struct PMIU_cmd pmi_response;
    if (val) {
        if (sync) {
            pmi_errno = PMIU_msg_set_response_kvsget(pmi, &pmi_response, is_static, val, true);
        } else {
            pmi_errno = PMIU_msg_set_response_get(pmi, &pmi_response, is_static, val, true);
        }
    } else {
        pmi_errno = PMIU_msg_set_response_fail(pmi, &pmi_response, is_static, 1, "key_not_found");
    }
    HYDU_ASSERT(!pmi_errno, status);

    status = HYD_pmiserv_pmi_reply(proxy, process_fd, &pmi_response);
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmiserv_kvs_put(struct HYD_proxy *proxy, int process_fd, int pgid,
                               struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    int pmi_errno;
    HYDU_FUNC_ENTER();

    const char *kvsname, *key, *val;
    pmi_errno = PMIU_msg_get_query_put(pmi, &kvsname, &key, &val);
    HYDU_ASSERT(!pmi_errno, status);

    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg = PMISERV_pg_by_id(proxy->pgid);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    status = HYD_pmcd_pmi_add_kvs(key, val, pg_scratch->kvs, HYD_server_info.user_global.debug);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = HYD_pmiserv_pmi_reply(proxy, process_fd, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

    status = check_pending_reqs(key);
    HYDU_ERR_POP(status, "check_pending_reqs failed\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

/* NOTE: this is an aggregate put from proxy with multiple key=val pairs */
HYD_status HYD_pmiserv_kvs_mput(struct HYD_proxy *proxy, int process_fd, int pgid,
                                struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg = PMISERV_pg_by_id(proxy->pgid);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* FIXME: leak of pmi's abstraction */
    for (int i = 0; i < pmi->num_tokens; i++) {
        status = HYD_pmcd_pmi_add_kvs(pmi->tokens[i].key, pmi->tokens[i].val,
                                      pg_scratch->kvs, HYD_server_info.user_global.debug);
        HYDU_ERR_POP(status, "unable to add key pair to kvs\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmiserv_kvs_fence(struct HYD_proxy *proxy, int process_fd, int pgid,
                                 struct PMIU_cmd *pmi)
{
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    int pmi_errno;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg = PMISERV_pg_by_id(proxy->pgid);
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    int cur_epoch = -1;
    /* Try to find the epoch point of this process */
    for (i = 0; i < pg->pg_process_count; i++) {
        if (pg_scratch->ecount[i].process_fd == process_fd) {
            pg_scratch->ecount[i].epoch++;
            cur_epoch = pg_scratch->ecount[i].epoch;
            break;
        }
    }

    if (i == pg->pg_process_count) {
        /* couldn't find the current process; find a NULL entry */

        for (i = 0; i < pg->pg_process_count; i++)
            if (pg_scratch->ecount[i].epoch == -1)
                break;

        pg_scratch->ecount[i].process_fd = process_fd;
        pg_scratch->ecount[i].epoch = 0;
        cur_epoch = pg_scratch->ecount[i].epoch;
    }

    struct PMIU_cmd pmi_response;
    pmi_errno = PMIU_msg_set_response(pmi, &pmi_response, is_static);
    HYDU_ASSERT(!pmi_errno, status);

    status = HYD_pmiserv_pmi_reply(proxy, process_fd, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

    if (cur_epoch == pg_scratch->epoch) {
        pg_scratch->fence_count++;
        if (pg_scratch->fence_count % pg->pg_process_count == 0) {
            /* Poke the progress engine before exiting */
            status = check_pending_reqs(NULL);
            HYDU_ERR_POP(status, "check pending requests error\n");
            /* reset fence_count */
            pg_scratch->epoch++;
            pg_scratch->fence_count = 0;
            for (i = 0; i < pg->pg_process_count; i++) {
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

/* routines for epoch checking (PMI-v2 kvs-fence) */

HYD_status HYD_pmiserv_epoch_init(struct HYD_pg *pg)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = pg->pg_scratch;

    HYDU_MALLOC_OR_JUMP(pg_scratch->ecount, struct HYD_pmcd_pmi_ecount *,
                        pg->pg_process_count * sizeof(struct HYD_pmcd_pmi_ecount), status);
    /* initialize as sentinels. The first kvs-fence will fill the entry.
     * Subsequent kvs-fence will increment the epoch. */
    for (int i = 0; i < pg->pg_process_count; i++) {
        pg_scratch->ecount[i].process_fd = -1;
        pg_scratch->ecount[i].epoch = -1;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;
  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmiserv_epoch_free(struct HYD_pg *pg)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = pg->pg_scratch;

    MPL_free(pg_scratch->ecount);

    return HYD_SUCCESS;
}

static bool check_epoch_reached(struct HYD_pg *pg, int process_fd)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    int idx = -1;
    for (int i = 0; i < pg->pg_process_count; i++) {
        if (pg_scratch->ecount[i].process_fd == process_fd) {
            idx = i;
            break;
        }
    }
    assert(idx != -1);

    for (int i = 0; i < pg->pg_process_count; i++) {
        if (pg_scratch->ecount[i].epoch < pg_scratch->ecount[idx].epoch) {
            return false;
        }
    }

    return true;
}

static HYD_status HYD_pmcd_pmi_v2_queue_req(struct HYD_proxy *proxy, int process_fd, int pgid,
                                            struct PMIU_cmd *pmi, const char *key)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC_OR_JUMP(req, struct HYD_pmcd_pmi_v2_reqs *, sizeof(struct HYD_pmcd_pmi_v2_reqs),
                        status);
    req->proxy = proxy;
    req->process_fd = process_fd;
    req->pgid = pgid;
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

/* Process the pending get with matching key. If the key is NULL,
 * we are in a kvs-fence, clear all the pending reqs.
 */
static HYD_status check_pending_reqs(const char *key)
{
    struct HYD_pmcd_pmi_v2_reqs *req, *list_head = NULL, *list_tail = NULL;
    HYD_status status = HYD_SUCCESS;

    int count = 0;
    int has_pending = 0;
    for (req = pending_reqs; req; req = req->next) {
        count++;
        if (key && strcmp(key, req->key) == 0) {
            has_pending++;
        }
    }

    if (key && !has_pending) {
        goto fn_exit;
    }

    /* FIXME: this dequeue then enqueue is a bit silly. */
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
            status = HYD_pmiserv_kvs_get(req->proxy, req->process_fd, req->pgid, req->pmi, true);
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
