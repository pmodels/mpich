/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "pmi_handle.h"

static HYD_status fn_fullinit(int fd, int pgid, char *args[]);
static HYD_status fn_job_getid(int fd, int pgid, char *args[]);
static HYD_status fn_info_putnodeattr(int fd, int pgid, char *args[]);
static HYD_status fn_info_getnodeattr(int fd, int pgid, char *args[]);
static HYD_status fn_info_getjobattr(int fd, int pgid, char *args[]);
static HYD_status fn_kvs_put(int fd, int pgid, char *args[]);
static HYD_status fn_kvs_get(int fd, int pgid, char *args[]);
static HYD_status fn_kvs_fence(int fd, int pgid, char *args[]);
static HYD_status fn_finalize(int fd, int pgid, char *args[]);

static struct reqs {
    enum type {
        NODE_ATTR_GET,
        KVS_GET
    } type;

    int fd;
    int pgid;
    char *thrid;
    char **args;

    struct reqs *next;
} *pending_reqs = NULL;

static HYD_status send_command(int fd, char *cmd)
{
    char cmdlen[7];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

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

static int req_complete = 0;

static void free_req(struct reqs *req)
{
    HYDU_free_strlist(req->args);
    HYDU_FREE(req);
}

static HYD_status queue_req(int fd, int pgid, enum type type, char *args[])
{
    struct reqs *req, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC(req, struct reqs *, sizeof(struct reqs), status);
    req->fd = fd;
    req->pgid = pgid;
    req->type = type;
    req->next = NULL;

    status = HYDU_strdup_list(args, &req->args);
    HYDU_ERR_POP(status, "unable to dup args\n");

    if (pending_reqs == NULL)
        pending_reqs = req;
    else {
        for (tmp = pending_reqs; tmp->next; tmp = tmp->next);
        tmp->next = req;
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static struct reqs *dequeue_req_head(void)
{
    struct reqs *req = pending_reqs;

    if (pending_reqs) {
        pending_reqs = pending_reqs->next;
        req->next = NULL;
    }

    return req;
}

static HYD_status poke_progress(void)
{
    struct reqs *req;
    int i, count;
    HYD_status status = HYD_SUCCESS;

    for (count = 0, req = pending_reqs; req; req = req->next)
        count++;

    for (i = 0; i < count; i++) {
        req = dequeue_req_head();

        req_complete = 0;
        if (req->type == NODE_ATTR_GET) {
            status = fn_info_getnodeattr(req->fd, req->pgid, req->args);
            HYDU_ERR_POP(status, "getnodeattr returned error\n");
        }
        else if (req->type == KVS_GET) {
            status = fn_kvs_get(req->fd, req->pgid, req->args);
            HYDU_ERR_POP(status, "kvs_get returned error\n");
        }

        free_req(req);
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

static void print_req_list(void) ATTRIBUTE((unused));
static void print_req_list(void)
{
    struct reqs *req;

    if (pending_reqs)
        HYDU_dump_noprefix(stdout, "(");
    for (req = pending_reqs; req; req = req->next)
        HYDU_dump_noprefix(stdout, "%s ",
                           (req->type == NODE_ATTR_GET) ? "NODE_ATTR_GET" : "KVS_GET");
    if (pending_reqs)
        HYDU_dump_noprefix(stdout, ")\n");
}

static HYD_status fn_fullinit(int fd, int pgid, char *args[])
{
    int id, rank, i;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *rank_str;
    struct HYD_pg *pg;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    rank_str = HYD_pmcd_find_token_keyval(tokens, token_count, "pmirank");
    HYDU_ERR_CHKANDJUMP(status, rank_str == NULL, HYD_INTERNAL_ERROR,
                        "unable to find pmirank token\n");
    id = atoi(rank_str);

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=fullinit-response;pmi-version=2;pmi-subversion=0;rank=");

    status = HYD_pmcd_pmi_id_to_rank(id, pgid, &rank);
    HYDU_ERR_POP(status, "unable to convert ID to rank\n");
    tmp[i++] = HYDU_int_to_str(rank);

    tmp[i++] = HYDU_strdup(";size=");
    tmp[i++] = HYDU_int_to_str(HYD_handle.pg_list.pg_process_count);
    tmp[i++] = HYDU_strdup(";appnum=0;debugged=FALSE;pmiverbose=0;rc=0;");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "error while joining strings\n");

    for (i = 0; tmp[i]; i++)
        HYDU_FREE(tmp[i]);

    status = send_command(fd, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);

    for (pg = &HYD_handle.pg_list; pg && pg->pgid != pgid; pg = pg->next);
    if (!pg)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "could not find pg with pgid %d\n", pgid);

    /* Add the process to the appropriate PG */
    status = HYD_pmcd_pmi_add_process_to_pg(pg, fd, rank);
    HYDU_ERR_POP(status, "unable to add process to pg\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_job_getid(int fd, int pgid, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *thrid;
    int i;
    struct HYD_pmcd_pmi_process *process;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) process->proxy->pg->pg_scratch;

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=job-getid-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    tmp[i++] = HYDU_strdup("jobid=");
    tmp[i++] = HYDU_strdup(pg_scratch->kvs->kvs_name);
    tmp[i++] = HYDU_strdup(";rc=0;");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "unable to join strings\n");

    HYDU_free_strlist(tmp);

    status = send_command(fd, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_putnodeattr(int fd, int pgid, char *args[])
{
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    char *key, *val, *thrid;
    int i, ret;
    struct HYD_pmcd_pmi_process *process;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    val = HYD_pmcd_find_token_keyval(tokens, token_count, "value");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find value token\n");

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) process->proxy->proxy_scratch;

    status = HYD_pmcd_pmi_add_kvs(key, val, proxy_scratch->kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=info-putnodeattr-response;");
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

    status = send_command(fd, cmd);
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

static HYD_status fn_info_getnodeattr(int fd, int pgid, char *args[])
{
    int i, found;
    struct HYD_pmcd_pmi_process *process;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    char *key, *waitval, *thrid;
    char *tmp[HYD_NUM_TMP_STRINGS] = { 0 }, *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    waitval = HYD_pmcd_find_token_keyval(tokens, token_count, "wait");
    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) process->proxy->proxy_scratch;

    found = 0;
    for (run = proxy_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (found) {        /* We found the attribute */
        i = 0;
        tmp[i++] = HYDU_strdup("cmd=info-getnodeattr-response;");
        if (thrid) {
            tmp[i++] = HYDU_strdup("thrid=");
            tmp[i++] = HYDU_strdup(thrid);
            tmp[i++] = HYDU_strdup(";");
        }
        tmp[i++] = HYDU_strdup("found=TRUE;value=");
        tmp[i++] = HYDU_strdup(run->val);
        tmp[i++] = HYDU_strdup(";rc=0;");
        tmp[i++] = NULL;

        status = HYDU_str_alloc_and_join(tmp, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(tmp);

        status = send_command(fd, cmd);
        HYDU_ERR_POP(status, "send command failed\n");
        HYDU_FREE(cmd);
    }
    else if (waitval && !strcmp(waitval, "TRUE")) {
        /* The client wants to wait for a response; queue up the request */
        status = queue_req(fd, pgid, NODE_ATTR_GET, args);
        HYDU_ERR_POP(status, "unable to queue request\n");

        goto fn_exit;
    }
    else {
        /* Tell the client that we can't find the attribute */
        i = 0;
        tmp[i++] = HYDU_strdup("cmd=info-getnodeattr-response;");
        if (thrid) {
            tmp[i++] = HYDU_strdup("thrid=");
            tmp[i++] = HYDU_strdup(thrid);
            tmp[i++] = HYDU_strdup(";");
        }
        tmp[i++] = HYDU_strdup("found=FALSE;rc=0;");
        tmp[i++] = NULL;

        status = HYDU_str_alloc_and_join(tmp, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(tmp);

        status = send_command(fd, cmd);
        HYDU_ERR_POP(status, "send command failed\n");
        HYDU_FREE(cmd);
    }

    /* Mark the global completion variable, in case the progress
     * engine is monitoring. */
    /* FIXME: This should be an output parameter. We need to change
     * the structure of the PMI function table to be able to take
     * additional arguments, and not just the ones passed on the
     * wire. */
    req_complete = 1;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_info_getjobattr(int fd, int pgid, char *args[])
{
    int i, ret;
    struct HYD_pmcd_pmi_process *process;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *key;
    char *thrid;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *node_list;
    struct HYD_pmcd_token *tokens;
    int token_count, found;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) process->proxy->pg->pg_scratch;

    /* Try to find the key */
    found = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (found == 0) {
        /* Didn't find the job attribute; see if we know how to
         * generate it */
        if (strcmp(key, "PMI_process_mapping") == 0) {
            /* Create a vector format */
            status = HYD_pmcd_pmi_process_mapping(process, &node_list);
            HYDU_ERR_POP(status, "Unable to get process mapping information\n");

            /* Make sure the node list is within the size allowed by
             * PMI. Otherwise, tell the client that we don't know what
             * the key is */
            if (strlen(node_list) <= MAXVALLEN) {
                status =
                    HYD_pmcd_pmi_add_kvs("PMI_process_mapping", node_list, pg_scratch->kvs,
                                         &ret);
                HYDU_ERR_POP(status, "unable to add process_mapping to KVS\n");
            }

            HYDU_FREE(node_list);
        }

        /* Search for the key again */
        for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
            if (!strcmp(run->key, key)) {
                found = 1;
                break;
            }
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

    status = send_command(fd, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_put(int fd, int pgid, char *args[])
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

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    val = HYD_pmcd_find_token_keyval(tokens, token_count, "value");
    if (val == NULL) {
        /* the user sent an empty string */
        val = HYDU_strdup("");
    }

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

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

    status = send_command(fd, cmd);
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

static HYD_status fn_kvs_get(int fd, int pgid, char *args[])
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

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

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
            status = queue_req(fd, pgid, KVS_GET, args);
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

    status = send_command(fd, cmd);
    HYDU_ERR_POP(status, "send command failed\n");

    HYDU_FREE(cmd);
    req_complete = 1;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_kvs_fence(int fd, int pgid, char *args[])
{
    struct HYD_pmcd_pmi_process *process;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *thrid;
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d\n", fd);

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

    status = send_command(fd, cmd);
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

static HYD_status fn_finalize(int fd, int pgid, char *args[])
{
    char *thrid;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_find_token_keyval(tokens, token_count, "thrid");

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=finalize-response;");
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

    status = send_command(fd, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

    if (status == HYD_SUCCESS) {
        status = HYDT_dmx_deregister_fd(fd);
        HYDU_ERR_POP(status, "unable to register fd\n");
        close(fd);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* TODO: abort, create_kvs, destroy_kvs, getbyidx, spawn */
static struct HYD_pmcd_pmi_handle_fns pmi_v2_handle_fns_foo[] = {
    {"fullinit", fn_fullinit},
    {"job-getid", fn_job_getid},
    {"info-putnodeattr", fn_info_putnodeattr},
    {"info-getnodeattr", fn_info_getnodeattr},
    {"info-getjobattr", fn_info_getjobattr},
    {"kvs-put", fn_kvs_put},
    {"kvs-get", fn_kvs_get},
    {"kvs-fence", fn_kvs_fence},
    {"finalize", fn_finalize},
    {"\0", NULL}
};

struct HYD_pmcd_pmi_handle HYD_pmcd_pmi_v2 = { pmi_v2_handle_fns_foo };
