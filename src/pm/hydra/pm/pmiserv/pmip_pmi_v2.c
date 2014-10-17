/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmip_pmi.h"
#include "pmip.h"
#include "bsci.h"
#include "demux.h"
#include "topo.h"
#include "pmi_v2_common.h"

static HYD_status fn_info_getnodeattr(int fd, char *args[]);

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status send_cmd_upstream(const char *start, int fd, char *args[])
{
    int i, sent, closed;
    struct HYD_string_stash stash;
    char *buf = NULL;
    struct HYD_pmcd_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup(start), status);
    for (i = 0; args[i]; i++) {
        HYD_STRING_STASH(stash, HYDU_strdup(args[i]), status);
        if (args[i + 1])
            HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }

    HYD_STRING_SPIT(stash, buf, status);

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PMI_CMD;
    hdr.pid = fd;
    hdr.buflen = strlen(buf);
    hdr.pmi_version = 2;
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
    HYDU_ASSERT(!closed, status);

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "forwarding command (%s) upstream\n", buf);
    }

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed,
                             HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    if (buf)
        HYDU_FREE(buf);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status send_cmd_downstream(int fd, const char *cmd)
{
    char cmdlen[7];
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_snprintf(cmdlen, 7, "%6u", (unsigned) strlen(cmd));
    status = HYDU_sock_write(fd, cmdlen, 6, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    /* FIXME: We cannot abort when we are not able to send data
     * downstream. The upper layer needs to handle this based on
     * whether we want to abort or not.*/
    HYDU_ASSERT(!closed, status);

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "PMI response: %s\n", cmd);
    }

    status = HYDU_sock_write(fd, cmd, strlen(cmd), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status poke_progress(char *key)
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

        if (key && strcmp(key, req->key)) {
            /* If the key doesn't match the request, just queue it back */
            if (list_head == NULL) {
                list_head = req;
                list_tail = req;
            }
            else {
                list_tail->next = req;
                req->prev = list_tail;
                list_tail = req;
            }
        }
        else {
            status = fn_info_getnodeattr(req->fd, req->args);
            HYDU_ERR_POP(status, "getnodeattr returned error\n");

            /* Free the dequeued request */
            HYDU_free_strlist(req->args);
            HYDU_FREE(req);
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

static HYD_status fn_fullinit(int fd, char *args[])
{
    int id, i;
    char *rank_str;
    struct HYD_string_stash stash;
    char *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    rank_str = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "pmirank");
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
    HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash,
                     HYDU_strdup("cmd=fullinit-response;pmi-version=2;pmi-subversion=0;rank="),
                     status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(id), status);

    HYD_STRING_STASH(stash, HYDU_strdup(";size="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_process_count),
                     status);
    HYD_STRING_STASH(stash, HYDU_strdup(";appnum=0"), status);
    if (HYD_pmcd_pmip.local.spawner_kvsname) {
        HYD_STRING_STASH(stash, HYDU_strdup(";spawner-jobid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(HYD_pmcd_pmip.local.spawner_kvsname), status);
    }
    if (HYD_pmcd_pmip.user_global.debug) {
        HYD_STRING_STASH(stash, HYDU_strdup(";debugged=TRUE;pmiverbose=TRUE"), status);
    }
    else {
        HYD_STRING_STASH(stash, HYDU_strdup(";debugged=FALSE;pmiverbose=FALSE"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup(";rc=0;"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    send_cmd_downstream(fd, cmd);
    HYDU_FREE(cmd);

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_job_getid(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd, *thrid;
    struct HYD_pmcd_token *tokens = NULL;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=job-getid-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("jobid="), status);
    HYD_STRING_STASH(stash, HYDU_strdup(HYD_pmcd_pmip.local.kvs->kvsname), status);
    HYD_STRING_STASH(stash, HYDU_strdup(";rc=0;"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    send_cmd_downstream(fd, cmd);
    HYDU_FREE(cmd);

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_putnodeattr(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *key, *val, *thrid, *cmd;
    struct HYD_pmcd_token *tokens = NULL;
    int token_count, ret;
    struct HYD_pmcd_pmi_v2_reqs *req;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "value");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR, "unable to find value token\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    status = HYD_pmcd_pmi_add_kvs(key, val, HYD_pmcd_pmip.local.kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=info-putnodeattr-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("rc="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(ret), status);
    HYD_STRING_STASH(stash, HYDU_strdup(";"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    send_cmd_downstream(fd, cmd);
    HYDU_FREE(cmd);

    for (req = pending_reqs; req; req = req->next) {
        if (!strcmp(req->key, key)) {
            /* Poke the progress engine before exiting */
            status = poke_progress(key);
            HYDU_ERR_POP(status, "poke progress error\n");
            break;
        }
    }

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_getnodeattr(int fd, char *args[])
{
    int found;
    struct HYD_pmcd_pmi_kvs_pair *run;
    char *key, *waitval, *thrid;
    struct HYD_string_stash stash;
    char *cmd;
    struct HYD_pmcd_token *tokens = NULL;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    waitval = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "wait");
    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

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
        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("cmd=info-getnodeattr-response;"), status);
        if (thrid) {
            HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
            HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
            HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
        }
        HYD_STRING_STASH(stash, HYDU_strdup("found=TRUE;value="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(run->val), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";rc=0;"), status);

        HYD_STRING_SPIT(stash, cmd, status);

        send_cmd_downstream(fd, cmd);
        HYDU_FREE(cmd);
    }
    else if (waitval && !strcmp(waitval, "TRUE")) {
        /* The client wants to wait for a response; queue up the request */
        status = HYD_pmcd_pmi_v2_queue_req(fd, -1, -1, args, key, &pending_reqs);
        HYDU_ERR_POP(status, "unable to queue request\n");

        goto fn_exit;
    }
    else {
        /* Tell the client that we can't find the attribute */
        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("cmd=info-getnodeattr-response;"), status);
        if (thrid) {
            HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
            HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
            HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
        }
        HYD_STRING_STASH(stash, HYDU_strdup("found=FALSE;rc=0;"), status);

        HYD_STRING_SPIT(stash, cmd, status);

        send_cmd_downstream(fd, cmd);
        HYDU_FREE(cmd);
    }

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_getjobattr(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd, *key, *thrid;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    if (!strcmp(key, "PMI_process_mapping")) {
        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("cmd=info-getjobattr-response;"), status);
        if (thrid) {
            HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
            HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
            HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
        }
        HYD_STRING_STASH(stash, HYDU_strdup("found=TRUE;value="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(HYD_pmcd_pmip.system_global.pmi_process_mapping),
                         status);
        HYD_STRING_STASH(stash, HYDU_strdup(";rc=0;"), status);

        HYD_STRING_SPIT(stash, cmd, status);

        send_cmd_downstream(fd, cmd);
        HYDU_FREE(cmd);
    }
    else {
        status = send_cmd_upstream("cmd=info-getjobattr;", fd, args);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_finalize(int fd, char *args[])
{
    char *thrid;
    struct HYD_string_stash stash;
    char *cmd;
    struct HYD_pmcd_token *tokens = NULL;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=finalize-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("rc=0;"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    send_cmd_downstream(fd, cmd);
    HYDU_FREE(cmd);

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
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
