/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "pmiserv_pmi.h"
#include "pmi_v2_common.h"

static HYD_status fn_fullinit(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_info_getjobattr(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_kvs_put(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_kvs_get(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_kvs_fence(int fd, int pid, int pgid, char *args[]);

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status cmd_response(int fd, int pid, char *cmd)
{
    char cmdlen[7];
    enum HYD_pmcd_pmi_cmd c;
    struct HYD_pmcd_pmi_response_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    c = PMI_RESPONSE;
    status = HYDU_sock_write(fd, &c, sizeof(c));
    HYDU_ERR_POP(status, "unable to send PMI_RESPONSE command to proxy\n");

    hdr.pid = pid;
    hdr.buflen = 6 + strlen(cmd);
    status = HYDU_sock_write(fd, &hdr, sizeof(hdr));
    HYDU_ERR_POP(status, "unable to send PMI_RESPONSE header to proxy\n");

    HYDU_snprintf(cmdlen, 7, "%6u", (unsigned) strlen(cmd));
    status = HYDU_sock_write(fd, cmdlen, 6);
    HYDU_ERR_POP(status, "error writing PMI line\n");

    status = HYDU_sock_write(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status poke_progress(void)
{
    struct HYD_pmcd_pmi_v2_reqs *req;
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

        status = fn_kvs_get(req->fd, req->pid, req->pgid, req->args);
        HYDU_ERR_POP(status, "kvs_get returned error\n");

        /* Free the dequeued request */
        HYDU_free_strlist(req->args);
        HYDU_FREE(req);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_pmcd_pmi_process *find_process(int fd, int pid)
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_pmcd_pmi_process *process = NULL;

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    if (proxy) {
        proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;
        if (proxy_scratch) {
            for (process = proxy_scratch->process_list; process; process = process->next)
                if (process->pid == pid)
                    return process;
        }
    }

    return NULL;
}

static HYD_status fn_fullinit(int fd, int pid, int pgid, char *args[])
{
    int id, i, rank;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *rank_str;
    struct HYD_pg *pg;
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

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=fullinit-response;pmi-version=2;pmi-subversion=0;rank=");
    status = HYD_pmcd_pmi_id_to_rank(id, pgid, &rank);
    HYDU_ERR_POP(status, "unable to translate PMI ID to rank\n");
    tmp[i++] = HYDU_int_to_str(rank);

    tmp[i++] = HYDU_strdup(";size=");
    tmp[i++] = HYDU_int_to_str(HYD_handle.pg_list.pg_process_count);
    tmp[i++] = HYDU_strdup(";appnum=0;debugged=FALSE;pmiverbose=0;rc=0;");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "error while joining strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);

    for (pg = &HYD_handle.pg_list; pg && pg->pgid != pgid; pg = pg->next);
    if (!pg)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "could not find pg with pgid %d\n",
                             pgid);

    status = HYD_pmcd_pmi_add_process_to_pg(pg, fd, pid, rank);
    HYDU_ERR_POP(status, "unable to add process to pg\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_getjobattr(int fd, int pid, int pgid, char *args[])
{
    int i;
    struct HYD_pmcd_pmi_process *process;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *key;
    char *thrid;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count, found;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) process->proxy->pg->pg_scratch;

    /* Try to find the key */
    found = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=info-getjobattr-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    tmp[i++] = HYDU_strdup("found=");
    if (found) {
        tmp[i++] = HYDU_strdup("TRUE;value=");
        tmp[i++] = HYDU_strdup(run->val);
        tmp[i++] = HYDU_strdup(";rc=0;");
    }
    else {
        tmp[i++] = HYDU_strdup("FALSE;rc=0;");
    }
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_put(int fd, int pid, int pgid, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    char *key, *val, *thrid;
    int i, ret;
    struct HYD_pmcd_pmi_process *process;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "value");
    if (val == NULL) {
        /* the user sent an empty string */
        val = HYDU_strdup("");
    }

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) process->proxy->pg->pg_scratch;

    status = HYD_pmcd_pmi_add_kvs(key, val, pg_scratch->kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=kvs-put-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    tmp[i++] = HYDU_strdup("rc=");
    tmp[i++] = HYDU_int_to_str(ret);
    tmp[i++] = HYDU_strdup(";");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);

    /* Poke the progress engine before exiting */
    status = poke_progress();
    HYDU_ERR_POP(status, "poke progress error\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_get(int fd, int pid, int pgid, char *args[])
{
    int i, found, barrier, process_count;
    struct HYD_pmcd_pmi_process *process, *prun;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_kvs_pair *run;
    char *key, *thrid;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) process->proxy->pg->pg_scratch;

    found = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        barrier = 1;
        process_count = 0;
        for (proxy = process->proxy->pg->proxy_list; proxy; proxy = proxy->next) {
            proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;

            if (proxy_scratch == NULL) {
                /* proxy not initialized yet */
                barrier = 0;
                break;
            }

            for (prun = proxy_scratch->process_list; prun; prun = prun->next) {
                process_count++;
                if (prun->epoch < process->epoch) {
                    barrier = 0;
                    break;
                }
            }
            if (!barrier)
                break;
        }

        if (!barrier || process_count < process->proxy->pg->pg_process_count) {
            /* We haven't reached a barrier yet; queue up request */
            status = HYD_pmcd_pmi_v2_queue_req(fd, pid, pgid, args, &pending_reqs);
            HYDU_ERR_POP(status, "unable to queue request\n");

            /* We are done */
            goto fn_exit;
        }
    }

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=kvs-get-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    if (found) {
        tmp[i++] = HYDU_strdup("found=TRUE;value=");
        tmp[i++] = HYDU_strdup(run->val);
        tmp[i++] = HYDU_strdup(";");
    }
    else {
        tmp[i++] = HYDU_strdup("found=FALSE;");
    }
    tmp[i++] = HYDU_strdup("rc=0;");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_fence(int fd, int pid, int pgid, char *args[])
{
    struct HYD_pmcd_pmi_process *process;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *thrid;
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    process->epoch++;   /* We have reached the next epoch */

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=kvs-fence-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    tmp[i++] = HYDU_strdup("rc=0;");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

    /* Poke the progress engine before exiting */
    status = poke_progress();
    HYDU_ERR_POP(status, "poke progress error\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* TODO: abort, create_kvs, destroy_kvs, getbyidx, spawn */
static struct HYD_pmcd_pmi_handle pmi_v2_handle_fns_foo[] = {
    {"fullinit", fn_fullinit},
    {"info-getjobattr", fn_info_getjobattr},
    {"kvs-put", fn_kvs_put},
    {"kvs-get", fn_kvs_get},
    {"kvs-fence", fn_kvs_fence},
    {"\0", NULL}
};

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_v2 = pmi_v2_handle_fns_foo;
