/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "bsci.h"
#include "pmiserv.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"
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

    if (HYD_handle.user_global.debug) {
        HYDU_dump(stdout, "PMI response to fd %d pid %d: %s\n", fd, pid, cmd);
    }

    status = HYDU_sock_write(fd, cmd, strlen(cmd));
    HYDU_ERR_POP(status, "error writing PMI line\n");

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
            status = fn_kvs_get(req->fd, req->pid, req->pgid, req->args);
            HYDU_ERR_POP(status, "kvs_get returned error\n");

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

static HYD_status fn_fullinit(int fd, int pid, int pgid, char *args[])
{
    int id, i, rank;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *rank_str;
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
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

    for (pg = &HYD_handle.pg_list; pg && pg->pgid != pgid; pg = pg->next);
    if (!pg)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "unable to find pgid %d\n", pgid);

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=fullinit-response;pmi-version=2;pmi-subversion=0;rank=");
    status = HYD_pmcd_pmi_id_to_rank(id, pgid, &rank);
    HYDU_ERR_POP(status, "unable to translate PMI ID to rank\n");
    tmp[i++] = HYDU_int_to_str(rank);

    tmp[i++] = HYDU_strdup(";size=");
    tmp[i++] = HYDU_int_to_str(pg->pg_process_count);
    tmp[i++] = HYDU_strdup(";appnum=0");
    if (pg->spawner_pg) {
        tmp[i++] = HYDU_strdup(";spawner-jobid=");
        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->spawner_pg->pg_scratch;
        tmp[i++] = HYDU_strdup(pg_scratch->kvs->kvs_name);
    }
    if (HYD_handle.user_global.debug) {
        tmp[i++] = HYDU_strdup(";debugged=TRUE;pmiverbose=TRUE");
    }
    else {
        tmp[i++] = HYDU_strdup(";debugged=FALSE;pmiverbose=FALSE");
    }
    tmp[i++] = HYDU_strdup(";rc=0;");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &cmd);
    HYDU_ERR_POP(status, "error while joining strings\n");
    HYDU_free_strlist(tmp);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

    status = HYD_pmcd_pmi_add_process_to_pg(pg, fd, pid, rank);
    HYDU_ERR_POP(status, "unable to add process to pg\n");

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
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
    process = HYD_pmcd_pmi_find_process(fd, pid);
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
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
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
    struct HYD_pmcd_pmi_v2_reqs *req;
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
    process = HYD_pmcd_pmi_find_process(fd, pid);
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

    for (req = pending_reqs; req; req = req->next) {
        if (!strcmp(req->key, key)) {
            /* Poke the progress engine before exiting */
            status = poke_progress(key);
            HYDU_ERR_POP(status, "poke progress error\n");
            break;
        }
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
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
    process = HYD_pmcd_pmi_find_process(fd, pid);
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
            status = HYD_pmcd_pmi_v2_queue_req(fd, pid, pgid, args, key, &pending_reqs);
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
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
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
    static int fence_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd, pid);
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

    fence_count++;
    if (fence_count % process->proxy->pg->pg_process_count == 0) {
        /* Poke the progress engine before exiting */
        status = poke_progress(NULL);
        HYDU_ERR_POP(status, "poke progress error\n");
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static void segment_tokens(struct HYD_pmcd_token *tokens, int token_count,
                           struct HYD_pmcd_token_segment *segment_list, int *num_segments)
{
    int i, j;

    j = 0;
    segment_list[j].start_idx = 0;
    segment_list[j].token_count = 0;
    for (i = 0; i < token_count; i++) {
        if (!strcmp(tokens[i].key, "subcmd")) {
            j++;
            segment_list[j].start_idx = i;
            segment_list[j].token_count = 1;
        }
        else {
            segment_list[j].token_count++;
        }
    }
    *num_segments = j + 1;
}

static HYD_status fn_spawn(int fd, int pid, int pgid, char *args[])
{
    struct HYD_pg *pg;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_node *node_list = NULL, *node, *tnode;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_token *tokens;
    struct HYD_exec *exec_list = NULL, *exec;
    struct HYD_env *env;
    struct HYD_pmcd_pmi_process *process;

    char *thrid;
    char *key, *val;
    int maxprocs, preputcount, infokeycount, ret;
    int ncmds;
    char *execname, *path = NULL;

    struct HYD_pmcd_token_segment *segment_list = NULL;

    int token_count, i, j, k, pmi_id = -1, new_pgid, offset;
    int argcnt, num_segments;
    char *pmi_port = NULL, *control_port, *proxy_args[HYD_NUM_TMP_STRINGS] = { NULL };
    char *tmp[HYD_NUM_TMP_STRINGS];
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");


    /* Here's the order of things we do:
     *
     *   1. Break the token list into multiple segments, each segment
     *      corresponding to a command. Each command represents
     *      information for one executable.
     *
     *   2. Allocate a process group for the new set of spawned
     *      processes
     *
     *   3. Get all the common keys and deal with them
     *
     *   4. Create an executable list based on the segments.
     *
     *   5. Create a proxy list using the created executable list and
     *      spawn it.
     */

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "ncmds");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: ncmds\n");
    ncmds = atoi(val);

    HYDU_MALLOC(segment_list, struct HYD_pmcd_token_segment *,
                (ncmds + 1) * sizeof(struct HYD_pmcd_token_segment), status);
    segment_tokens(tokens, token_count, segment_list, &num_segments);
    HYDU_ASSERT((ncmds + 1) == num_segments, status);


    /* Allocate a new process group */
    for (pg = &HYD_handle.pg_list; pg->next; pg = pg->next);
    new_pgid = pg->pgid + 1;

    status = HYDU_alloc_pg(&pg->next, new_pgid);
    HYDU_ERR_POP(status, "unable to allocate process group\n");

    pg = pg->next;
    pg->pg_process_count = 0;

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    pg->spawner_pg = process->proxy->pg;

    for (j = 1; j <= ncmds; j++) {
        /* For each segment, we create an exec structure */
        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "maxprocs");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: maxprocs\n");
        maxprocs = atoi(val);
        pg->pg_process_count += maxprocs;

        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "argc");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: argc\n");
        argcnt = atoi(val);

        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "infokeycount");
        if (val)
            infokeycount = atoi(val);
        else
            infokeycount = 0;

        if (exec_list == NULL) {
            status = HYDU_alloc_exec(&exec_list);
            HYDU_ERR_POP(status, "unable to allocate exec\n");
            exec_list->appnum = 0;
            exec = exec_list;
        }
        else {
            for (exec = exec_list; exec->next; exec = exec->next);
            status = HYDU_alloc_exec(&exec->next);
            HYDU_ERR_POP(status, "unable to allocate exec\n");
            exec->next->appnum = exec->appnum + 1;
            exec = exec->next;
        }

        /* Info keys */
        for (i = 0; i < infokeycount; i++) {
            char *info_key, *info_val;

            HYDU_MALLOC(key, char *, MAXKEYLEN, status);
            HYDU_snprintf(key, MAXKEYLEN, "infokey%d", i);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP1(status, val == NULL, HYD_INTERNAL_ERROR,
                                 "unable to find token: %s\n", key);
            info_key = val;

            HYDU_snprintf(key, MAXKEYLEN, "infoval%d", i);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP1(status, val == NULL, HYD_INTERNAL_ERROR,
                                 "unable to find token: %s\n", key);
            info_val = val;

            if (!strcmp(info_key, "path")) {
                path = HYDU_strdup(info_val);
            }
            else if (!strcmp(info_key, "wdir")) {
                exec->wdir = HYDU_strdup(info_val);
            }
            else {
                /* FIXME: Unrecognized info key; what should we do
                 * here? Abort? */
                HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "unrecognized info key: %s\n",
                                     info_key);
            }
        }

        status = HYDU_correct_wdir(&exec->wdir);
        HYDU_ERR_POP(status, "unable to correct wdir\n");

        val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                             segment_list[j].token_count, "subcmd");
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: subcmd\n");
        if (path == NULL)
            execname = HYDU_strdup(val);
        else {
            i = 0;
            tmp[i++] = HYDU_strdup(path);
            tmp[i++] = HYDU_strdup("/");
            tmp[i++] = HYDU_strdup(val);
            tmp[i++] = NULL;

            status = HYDU_str_alloc_and_join(tmp, &execname);
            HYDU_ERR_POP(status, "error while joining strings\n");
            HYDU_free_strlist(tmp);
        }

        i = 0;
        exec->exec[i++] = execname;
        for (k = 0; k < argcnt; k++) {
            HYDU_MALLOC(key, char *, MAXKEYLEN, status);
            HYDU_snprintf(key, MAXKEYLEN, "argv%d", k);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP1(status, val == NULL, HYD_INTERNAL_ERROR,
                                 "unable to find token: %s\n", key);
            exec->exec[i++] = HYDU_strdup(val);
            HYDU_FREE(key);
        }
        exec->exec[i++] = NULL;

        exec->proc_count = maxprocs;

        /* It is not clear what kind of environment needs to get
         * passed to the spawned process. Don't set anything here, and
         * let the proxy do whatever it does by default. */
        exec->env_prop = NULL;

        status = HYDU_env_create(&env, "PMI_SPAWNED", (char *) "1");
        HYDU_ERR_POP(status, "unable to create PMI_SPAWNED environment\n");
        exec->user_env = env;
    }

    status = HYD_pmcd_pmi_alloc_pg_scratch(pg);
    HYDU_ERR_POP(status, "unable to allocate pg scratch space\n");

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;


    /* Get the common keys and deal with them */
    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "preputcount");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: preputcount\n");
    preputcount = atoi(val);

    for (i = 0; i < preputcount; i++) {
        char *preput_key, *preput_val;

        HYDU_MALLOC(key, char *, MAXKEYLEN, status);
        HYDU_snprintf(key, MAXKEYLEN, "ppkey%d", i);
        val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, key);
        HYDU_ERR_CHKANDJUMP1(status, val == NULL, HYD_INTERNAL_ERROR,
                             "unable to find token: %s\n", key);
        preput_key = val;

        HYDU_snprintf(key, HYD_TMP_STRLEN, "ppval%d", i);
        val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, key);
        HYDU_ERR_CHKANDJUMP1(status, val == NULL, HYD_INTERNAL_ERROR,
                             "unable to find token: %s\n", key);
        preput_val = val;
        HYDU_FREE(key);

        status = HYD_pmcd_pmi_add_kvs(preput_key, preput_val, pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add keypair to kvs\n");
    }


    /* Create the proxy list */
    offset = 0;
    for (pg = &HYD_handle.pg_list; pg->next; pg = pg->next)
        offset += pg->pg_process_count;

    status = HYDU_create_proxy_list(exec_list, HYD_handle.node_list, pg, offset);
    HYDU_ERR_POP(status, "error creating proxy list\n");
    HYDU_free_exec_list(exec_list);

    status = HYDU_sock_create_and_listen_portstr(HYD_handle.user_global.iface,
                                                 HYD_handle.port_range, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) new_pgid);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    /* Initialize PMI */
    ret = MPL_env2str("PMI_PORT", (const char **) &pmi_port);
    if (ret) {  /* PMI_PORT already set */
        if (HYD_handle.user_global.debug)
            HYDU_dump(stdout, "someone else already set PMI port\n");
        pmi_port = HYDU_strdup(pmi_port);

        ret = MPL_env2int("PMI_ID", &pmi_id);
        if (!ret)
            HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "PMI_PORT set but not PMI_ID\n");
    }
    else {
        pmi_id = -1;
    }
    if (HYD_handle.user_global.debug)
        HYDU_dump(stdout, "PMI port: %s; PMI ID: %d\n", pmi_port, pmi_id);

    /* Go to the last PG */
    for (pg = &HYD_handle.pg_list; pg->next; pg = pg->next);

    /* Copy the host list to pass to the bootstrap server */
    node_list = NULL;
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        HYDU_alloc_node(&node);
        node->hostname = HYDU_strdup(proxy->node.hostname);
        node->core_count = proxy->node.core_count;
        node->next = NULL;

        if (node_list == NULL) {
            node_list = node;
        }
        else {
            for (tnode = node_list; tnode->next; tnode = tnode->next);
            tnode->next = node;
        }
    }

    status = HYD_pmcd_pmi_fill_in_proxy_args(proxy_args, control_port, new_pgid);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");
    HYDU_FREE(control_port);

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pmi_port, pmi_id, pg);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");
    HYDU_FREE(pmi_port);

    status = HYDT_bsci_launch_procs(proxy_args, node_list, 0, HYD_handle.stdout_cb,
                                    HYD_handle.stderr_cb);
    HYDU_ERR_POP(status, "bootstrap server cannot launch processes\n");
    HYDU_free_node_list(node_list);

    {
        char *cmd_str[HYD_NUM_TMP_STRINGS], *cmd;

        i = 0;
        cmd_str[i++] = HYDU_strdup("cmd=spawn-response;");
        if (thrid) {
            cmd_str[i++] = HYDU_strdup("thrid=");
            cmd_str[i++] = HYDU_strdup(thrid);
            cmd_str[i++] = HYDU_strdup(";");
        }
        cmd_str[i++] = HYDU_strdup("rc=0;");
        cmd_str[i++] = HYDU_strdup("jobid=");
        cmd_str[i++] = HYDU_strdup(pg_scratch->kvs->kvs_name);
        cmd_str[i++] = HYDU_strdup(";");
        cmd_str[i++] = HYDU_strdup("nerrs=0;");
        cmd_str[i++] = NULL;

        status = HYDU_str_alloc_and_join(cmd_str, &cmd);
        HYDU_ERR_POP(status, "unable to join strings\n");
        HYDU_free_strlist(cmd_str);

        status = cmd_response(fd, pid, cmd);
        HYDU_ERR_POP(status, "send command failed\n");
        HYDU_FREE(cmd);
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_free_strlist(proxy_args);
    if (segment_list)
        HYDU_FREE(segment_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_publish(int fd, int pid, int pgid, char *args[])
{
    struct HYD_pmcd_pmi_process *process;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *thrid, *val;
    struct HYD_pmcd_pmi_publish *publish, *r;
    int i, token_count;
    struct HYD_pmcd_token *tokens;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    HYDU_MALLOC(publish, struct HYD_pmcd_pmi_publish *, sizeof(struct HYD_pmcd_pmi_publish),
                status);

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");
    publish->name = HYDU_strdup(val);

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "port")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: port\n");
    publish->port = HYDU_strdup(val);

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "infokeycount")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: infokeycount\n");
    if (val)
        publish->infokeycount = atoi(val);
    else
        publish->infokeycount = 0;

    HYDU_MALLOC(publish->info_keys, struct HYD_pmcd_pmi_info_keys *,
                sizeof(struct HYD_pmcd_pmi_info_keys), status);

    for (i = 0; i < publish->infokeycount; i++) {
        char *info_key, *info_val;

        HYDU_MALLOC(info_key, char *, MAXKEYLEN, status);
        HYDU_snprintf(info_key, MAXKEYLEN, "infokey%d", i);
        if ((info_val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, info_key)) == NULL)
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "cannot find token: %s\n",
                                 info_key);
        publish->info_keys[i].key = HYDU_strdup(info_val);

        HYDU_MALLOC(info_key, char *, MAXKEYLEN, status);
        HYDU_snprintf(info_key, MAXKEYLEN, "infoval%d", i);
        if ((info_val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, info_key)) == NULL)
            HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "cannot find token: %s\n",
                                 info_val);
        publish->info_keys[i].val = HYDU_strdup(info_val);
    }

    publish->next = NULL;

    if (HYD_pmcd_pmi_publish_list == NULL)
        HYD_pmcd_pmi_publish_list = publish;
    else {
        for (r = HYD_pmcd_pmi_publish_list; r->next; r = r->next);
        r->next = publish;
    }

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=name-publish-response;");
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

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_unpublish(int fd, int pid, int pgid, char *args[])
{
    struct HYD_pmcd_pmi_process *process;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *thrid, *name;
    struct HYD_pmcd_pmi_publish *publish, *r;
    int i, token_count, found = 0;
    struct HYD_pmcd_token *tokens;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    HYDU_MALLOC(publish, struct HYD_pmcd_pmi_publish *, sizeof(struct HYD_pmcd_pmi_publish),
                status);

    if ((name = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");

    if (HYD_pmcd_pmi_publish_list == NULL) {
        /* Can't find published service */
    }
    else if (!strcmp(HYD_pmcd_pmi_publish_list->name, name)) {
        publish = HYD_pmcd_pmi_publish_list;
        HYD_pmcd_pmi_publish_list = HYD_pmcd_pmi_publish_list->next;
        publish->next = NULL;

        HYD_pmcd_pmi_free_publish(publish);
        HYDU_FREE(publish);

        found = 1;
    }
    else {
        publish = HYD_pmcd_pmi_publish_list;
        do {
            if (publish->next == NULL)
                break;
            else if (!strcmp(publish->next->name, name)) {
                r = publish->next;
                publish->next = r->next;
                r->next = NULL;

                HYD_pmcd_pmi_free_publish(r);
                HYDU_FREE(r);

                found = 1;
            }
            else
                publish = publish->next;
        } while (1);
    }

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=name-unpublish-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    if (found)
        tmp[i++] = HYDU_strdup("rc=0;");
    else
        tmp[i++] = HYDU_strdup("rc=1;errmsg=service_not_found");
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

static HYD_status fn_name_lookup(int fd, int pid, int pgid, char *args[])
{
    struct HYD_pmcd_pmi_process *process;
    char *tmp[HYD_NUM_TMP_STRINGS], *cmd, *thrid, *name;
    struct HYD_pmcd_pmi_publish *publish;
    int i, token_count;
    struct HYD_pmcd_token *tokens;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    /* Find the group id corresponding to this fd */
    process = HYD_pmcd_pmi_find_process(fd, pid);
    if (process == NULL)        /* We didn't find the process */
        HYDU_ERR_SETANDJUMP2(status, HYD_INTERNAL_ERROR,
                             "unable to find process structure for fd %d and pid %d\n", fd,
                             pid);

    HYDU_MALLOC(publish, struct HYD_pmcd_pmi_publish *, sizeof(struct HYD_pmcd_pmi_publish),
                status);

    if ((name = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");

    for (publish = HYD_pmcd_pmi_publish_list; publish; publish = publish->next)
        if (!strcmp(publish->name, name))
            break;

    i = 0;
    tmp[i++] = HYDU_strdup("cmd=name-lookup-response;");
    if (thrid) {
        tmp[i++] = HYDU_strdup("thrid=");
        tmp[i++] = HYDU_strdup(thrid);
        tmp[i++] = HYDU_strdup(";");
    }
    if (publish) {
        tmp[i++] = HYDU_strdup("value=");
        tmp[i++] = HYDU_strdup(publish->port);
        tmp[i++] = HYDU_strdup(";found=TRUE;rc=0;");
    }
    else {
        tmp[i++] = HYDU_strdup("found=FALSE;rc=1;");
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

/* TODO: abort, create_kvs, destroy_kvs, getbyidx */
static struct HYD_pmcd_pmi_handle pmi_v2_handle_fns_foo[] = {
    {"fullinit", fn_fullinit},
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
