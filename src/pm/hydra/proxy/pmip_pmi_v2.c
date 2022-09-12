/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "pmip_pmi.h"
#include "pmip.h"
#include "bsci.h"
#include "demux.h"
#include "topo.h"
#include "pmi_v2_common.h"

static HYD_status fn_info_getnodeattr(int fd, struct PMIU_cmd *pmi);

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

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

static HYD_status fn_fullinit(int fd, struct PMIU_cmd *pmi)
{
    int id, i;
    const char *rank_str;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    rank_str = PMIU_cmd_find_keyval(pmi, "pmirank");
    HYDU_ERR_CHKANDJUMP(status, rank_str == NULL, HYD_INTERNAL_ERROR,
                        "unable to find pmirank token\n");
    id = atoi(rank_str);

    /* Store the PMI_RANK to fd mapping */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pmi_rank[i] == id) {
            HYD_pmcd_pmip.downstream.pmi_fd[i] = fd;
            HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 1;
            break;
        }
    }
    int idx = i;
    HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

    /* find executable information */
    i = 0;
    struct HYD_exec *exec;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {
        i += exec->proc_count;
        if (idx < i)
            break;
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "fullinit-response");
    PMIU_cmd_add_str(&pmi_response, "pmi-version", "2");
    PMIU_cmd_add_str(&pmi_response, "pmi-subversion", "0");
    PMIU_cmd_add_int(&pmi_response, "rank", id);
    PMIU_cmd_add_int(&pmi_response, "size", HYD_pmcd_pmip.system_global.global_process_count);
    PMIU_cmd_add_int(&pmi_response, "appnum", exec->appnum);
    if (HYD_pmcd_pmip.local.spawner_kvsname) {
        PMIU_cmd_add_str(&pmi_response, "spawner-jobid", HYD_pmcd_pmip.local.spawner_kvsname);
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_job_getid(int fd, struct PMIU_cmd *pmi)
{
    const char *thrid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "job-getid-response");
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

static HYD_status fn_info_putnodeattr(int fd, struct PMIU_cmd *pmi)
{
    const char *key, *val, *thrid;
    int ret;
    struct HYD_pmcd_pmi_v2_reqs *req;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    val = PMIU_cmd_find_keyval(pmi, "value");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR, "unable to find value token\n");

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    status = HYD_pmcd_pmi_add_kvs(key, val, HYD_pmcd_pmip.local.kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "info-putnodeattr");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

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

static HYD_status fn_info_getnodeattr(int fd, struct PMIU_cmd *pmi)
{
    int found;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *key, *waitval, *thrid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    waitval = PMIU_cmd_find_keyval(pmi, "wait");
    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    /* if a predefined value is not found, we let the code fall back
     * to regular search and return an error to the client */

    found = 0;
    for (run = HYD_pmcd_pmip.local.kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (found) {        /* We found the attribute */
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init(&pmi_response, 2, "info-getnodeattr-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_str(&pmi_response, "found", "TRUE");
        PMIU_cmd_add_str(&pmi_response, "value", run->val);
        PMIU_cmd_add_str(&pmi_response, "rc", "0");

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending command downstream\n");
    } else if (waitval && !strcmp(waitval, "TRUE")) {
        /* The client wants to wait for a response; queue up the request */
        status = HYD_pmcd_pmi_v2_queue_req(fd, -1, -1, pmi, key, &pending_reqs);
        HYDU_ERR_POP(status, "unable to queue request\n");

        goto fn_exit;
    } else {
        /* Tell the client that we can't find the attribute */
        struct PMIU_cmd pmi_response;
        PMIU_cmd_init(&pmi_response, 2, "info-getnodeattr-response");
        if (thrid) {
            PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
        }
        PMIU_cmd_add_str(&pmi_response, "found", "FALSE");
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

static HYD_status fn_info_getjobattr(int fd, struct PMIU_cmd *pmi)
{
    const char *key, *thrid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    struct PMIU_cmd pmi_response;
    if (!strcmp(key, "PMI_process_mapping")) {
        PMIU_cmd_init(&pmi_response, 2, "info-getjobattr-response");
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

        PMIU_cmd_init(&pmi_response, 2, "info-getjobattr-response");
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

static HYD_status fn_finalize(int fd, struct PMIU_cmd *pmi)
{
    const char *thrid;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    thrid = PMIU_cmd_find_keyval(pmi, "thrid");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 2, "finalize-response");
    if (thrid) {
        PMIU_cmd_add_str(&pmi_response, "thrid", thrid);
    }
    PMIU_cmd_add_str(&pmi_response, "rc", "0");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending command downstream\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_pmcd_pmip_pmi_handle pmi_v2_handle_fns_foo[] = {
    {"fullinit", fn_fullinit},
    {"job-getid", fn_job_getid},
    {"info-putnodeattr", fn_info_putnodeattr},
    {"info-getnodeattr", fn_info_getnodeattr},
    {"info-getjobattr", fn_info_getjobattr},
    {"finalize", fn_finalize},
    {"\0", NULL}
};

struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_v2 = pmi_v2_handle_fns_foo;
