/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "hydra_server.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"
#include "pmi_v2_common.h"

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static bool check_epoch_reached(struct HYD_pg *pg, int fd, int pid);
static HYD_status check_pending_reqs(const char *key);

HYD_status HYD_pmiserv_kvs_get(int fd, int pid, int pgid, struct PMIU_cmd *pmi, bool sync)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_proxy *proxy;
    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    const char *thrid = NULL;
    const char *kvsname;
    const char *key;
    if (pmi->version == 1) {
        HYD_PMI_GET_STRVAL(pmi, "kvsname", kvsname);
        HYD_PMI_GET_STRVAL(pmi, "key", key);
        if (strcmp(pg_scratch->kvs->kvsname, kvsname)) {
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                                "kvsname (%s) does not match this group's kvs space (%s)\n",
                                kvsname, pg_scratch->kvs->kvsname);
        }
    } else {
        HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "thrid", thrid, NULL);
        HYD_PMI_GET_STRVAL(pmi, "key", key);
    }

    int found;
    const char *val;
    found = 0;
    val = NULL;
    if (strcmp(key, "PMI_dead_processes") == 0) {
        found = 1;
        val = pg_scratch->dead_processes;
    } else {
        struct HYD_pmcd_pmi_kvs_pair *run;
        for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
            if (strcmp(run->key, key) == 0) {
                found = 1;
                val = run->val;
                break;
            }
        }
    }

    if (!found && sync) {
        /* check whether all proxies have arrived at the same epoch or enqueue
         * the "get". A "put" (from another proxy) will poke the progress.
         */
        if (!check_epoch_reached(proxy->pg, fd, pid)) {
            status = HYD_pmcd_pmi_v2_queue_req(fd, pid, pgid, pmi, key, &pending_reqs);
            HYDU_ERR_POP(status, "unable to queue request\n");
            goto fn_exit;
        }
    }

    struct PMIU_cmd pmi_response;
    if (pmi->version == 1) {
        PMIU_cmd_init_static(&pmi_response, 1, "get_result");
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
    } else {
        if (strcmp(pmi->cmd, "kvs-get") == 0) {
            PMIU_cmd_init_static(&pmi_response, 2, "kvs-get-response");
        } else {
            PMIU_cmd_init_static(&pmi_response, 2, "info-getjobattr-response");
        }
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        if (found) {
            PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
            PMIU_cmd_add_str(&pmi_response, "value", val);
            PMIU_cmd_add_str(&pmi_response, "rc", "0");
        } else {
            PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
            PMIU_cmd_add_str(&pmi_response, "rc", "0");
        }
    }

    status = HYD_pmiserv_pmi_reply(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmiserv_kvs_put(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    const char *thrid = NULL;
    const char *key, *val;
    if (pmi->version == 1) {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PMI-v1 doesn't support server single key/val put\n");
    } else {
        HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "thrid", thrid, NULL);
        HYD_PMI_GET_STRVAL(pmi, "key", key);
        HYD_PMI_GET_STRVAL_WITH_DEFAULT(pmi, "value", val, "");
    }

    struct HYD_proxy *proxy;
    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    int ret;
    status = HYD_pmcd_pmi_add_kvs(key, val, pg_scratch->kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    struct PMIU_cmd pmi_response;
    if (pmi->version == 1) {
        assert(0);
    } else {
        PMIU_cmd_init_static(&pmi_response, 2, "kvs-put-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_int(&pmi_response, "rc", ret);
    }

    status = HYD_pmiserv_pmi_reply(fd, pid, &pmi_response);
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
HYD_status HYD_pmiserv_kvs_mput(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    struct HYD_proxy *proxy;
    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
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

HYD_status HYD_pmiserv_kvs_fence(int fd, int pid, int pgid, struct PMIU_cmd *pmi)
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
    PMIU_cmd_init_static(&pmi_response, 2, "kvs-fence-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = HYD_pmiserv_pmi_reply(fd, pid, &pmi_response);
    HYDU_ERR_POP(status, "send command failed\n");

    if (cur_epoch == pg_scratch->epoch) {
        pg_scratch->fence_count++;
        if (pg_scratch->fence_count % proxy->pg->pg_process_count == 0) {
            /* Poke the progress engine before exiting */
            status = check_pending_reqs(NULL);
            HYDU_ERR_POP(status, "check pending requests error\n");
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
        pg_scratch->ecount[i].fd = HYD_FD_UNSET;
        pg_scratch->ecount[i].pid = -1;
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

static bool check_epoch_reached(struct HYD_pg *pg, int fd, int pid)
{
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    int idx = -1;
    for (int i = 0; i < pg->pg_process_count; i++) {
        if (pg_scratch->ecount[i].fd == fd && pg_scratch->ecount[i].pid == pid) {
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
            status = HYD_pmiserv_kvs_get(req->fd, req->pid, req->pgid, req->pmi, true);
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
