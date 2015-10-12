/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_server.h"
#include "hydra.h"
#include "bsci.h"
#include "pmiserv.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"
#include "pmi_v2_common.h"

static HYD_status fn_info_getjobattr(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_kvs_put(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_kvs_get(int fd, int pid, int pgid, char *args[]);
static HYD_status fn_kvs_fence(int fd, int pid, int pgid, char *args[]);

static struct HYD_pmcd_pmi_v2_reqs *pending_reqs = NULL;

static HYD_status cmd_response(int fd, int pid, char *cmd)
{
    char cmdlen[7];
    struct HYD_pmcd_hdr hdr;
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PMI_RESPONSE;
    hdr.pid = pid;
    hdr.pmi_version = 2;
    hdr.buflen = 6 + strlen(cmd);
    status = HYDU_sock_write(fd, &hdr, sizeof(hdr), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI_RESPONSE header to proxy\n");
    HYDU_ASSERT(!closed, status);

    HYDU_snprintf(cmdlen, 7, "%6u", (unsigned) strlen(cmd));
    status = HYDU_sock_write(fd, cmdlen, 6, &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    HYDU_ASSERT(!closed, status);

    if (HYD_server_info.user_global.debug) {
        HYDU_dump(stdout, "PMI response to fd %d pid %d: %s\n", fd, pid, cmd);
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

static HYD_status fn_info_getjobattr(int fd, int pid, int pgid, char *args[])
{
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pmcd_pmi_kvs_pair *run;
    const char *key;
    char *thrid, *val, *cmd;
    struct HYD_string_stash stash;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    val = NULL;
    if (!strcmp(key, "PMI_dead_processes"))
        val = pg_scratch->dead_processes;

    /* Try to find the key */
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            val = run->val;
            break;
        }
    }

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=info-getjobattr-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("found="), status);
    if (val) {
        HYD_STRING_STASH(stash, HYDU_strdup("TRUE;value="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(val), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";rc=0;"), status);
    }
    else {
        HYD_STRING_STASH(stash, HYDU_strdup("FALSE;rc=0;"), status);
    }

    HYD_STRING_SPIT(stash, cmd, status);

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
    struct HYD_string_stash stash;
    char *key, *val, *thrid, *cmd;
    int ret;
    struct HYD_proxy *proxy;
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

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    status = HYD_pmcd_pmi_add_kvs(key, val, pg_scratch->kvs, &ret);
    HYDU_ERR_POP(status, "unable to put data into kvs\n");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=kvs-put-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("rc="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(ret), status);
    HYD_STRING_STASH(stash, HYDU_strdup(";"), status);

    HYD_STRING_SPIT(stash, cmd, status);

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
    int i, idx, found;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_kvs_pair *run;
    char *key, *thrid, *cmd;
    struct HYD_string_stash stash;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find key token\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    found = 0;
    for (run = pg_scratch->kvs->key_pair; run; run = run->next) {
        if (!strcmp(run->key, key)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        pg = proxy->pg;
        pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

        idx = -1;
        for (i = 0; i < pg->pg_process_count; i++)
            if (pg_scratch->ecount[i].fd == fd && pg_scratch->ecount[i].pid == pid) {
                idx = i;
                break;
            }

        HYDU_ASSERT(idx != -1, status);

        for (i = 0; i < pg->pg_process_count; i++) {
            if (pg_scratch->ecount[i].epoch < pg_scratch->ecount[idx].epoch) {
                /* We haven't reached a barrier yet; queue up request */
                status = HYD_pmcd_pmi_v2_queue_req(fd, pid, pgid, args, key, &pending_reqs);
                HYDU_ERR_POP(status, "unable to queue request\n");

                /* We are done */
                goto fn_exit;
            }
        }
    }

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=kvs-get-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    if (found) {
        HYD_STRING_STASH(stash, HYDU_strdup("found=TRUE;value="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(run->val), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    else {
        HYD_STRING_STASH(stash, HYDU_strdup("found=FALSE;"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("rc=0;"), status);

    HYD_STRING_SPIT(stash, cmd, status);

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
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_pg_scratch *pg_scratch;
    struct HYD_string_stash stash;
    char *cmd, *thrid;
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    static int fence_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) proxy->pg->pg_scratch;

    /* Try to find the epoch point of this process */
    for (i = 0; i < proxy->pg->pg_process_count; i++)
        if (pg_scratch->ecount[i].fd == fd && pg_scratch->ecount[i].pid == pid)
            pg_scratch->ecount[i].epoch++;

    if (i == proxy->pg->pg_process_count) {
        /* couldn't find the current process; find a NULL entry */

        for (i = 0; i < proxy->pg->pg_process_count; i++)
            if (pg_scratch->ecount[i].fd == HYD_FD_UNSET)
                break;

        pg_scratch->ecount[i].fd = fd;
        pg_scratch->ecount[i].pid = pid;
        pg_scratch->ecount[i].epoch = 1;
    }

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=kvs-fence-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    HYD_STRING_STASH(stash, HYDU_strdup("rc=0;"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

    fence_count++;
    if (fence_count % proxy->pg->pg_process_count == 0) {
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
    struct HYD_proxy *proxy;
    struct HYD_pmcd_token *tokens;
    struct HYD_exec *exec_list = NULL, *exec;
    struct HYD_env *env;
    struct HYD_node *node;

    char *thrid;
    char key[PMI_MAXKEYLEN], *val;
    int maxprocs, preputcount, infokeycount, ret;
    int ncmds;
    char *execname, *path = NULL;

    struct HYD_pmcd_token_segment *segment_list = NULL;

    int token_count, i, j, k, new_pgid;
    int argcnt, num_segments;
    struct HYD_string_stash proxy_stash;
    char *control_port;
    struct HYD_string_stash stash;
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
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR, "unable to find token: ncmds\n");
    ncmds = atoi(val);

    HYDU_MALLOC(segment_list, struct HYD_pmcd_token_segment *,
                (ncmds + 1) * sizeof(struct HYD_pmcd_token_segment), status);
    segment_tokens(tokens, token_count, segment_list, &num_segments);
    HYDU_ASSERT((ncmds + 1) == num_segments, status);


    /* Allocate a new process group */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);
    new_pgid = pg->pgid + 1;

    status = HYDU_alloc_pg(&pg->next, new_pgid);
    HYDU_ERR_POP(status, "unable to allocate process group\n");

    pg = pg->next;

    proxy = HYD_pmcd_pmi_find_proxy(fd);
    HYDU_ASSERT(proxy, status);

    pg->spawner_pg = proxy->pg;

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

            HYDU_snprintf(key, PMI_MAXKEYLEN, "infokey%d", i);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_key = val;

            HYDU_snprintf(key, PMI_MAXKEYLEN, "infoval%d", i);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            info_val = val;

            if (!strcmp(info_key, "path")) {
                path = HYDU_strdup(info_val);
            }
            else if (!strcmp(info_key, "wdir")) {
                exec->wdir = HYDU_strdup(info_val);
            }
            else if (!strcmp(info_key, "host") || !strcmp(info_key, "hosts")) {
                char *saveptr;
                char *host = strtok_r(info_val, ",", &saveptr);
                while (host) {
                    status = HYDU_process_mfile_token(host, 1, &pg->user_node_list);
                    HYDU_ERR_POP(status, "error creating node list\n");
                    host = strtok_r(NULL, ",", &saveptr);
                }
            }
            else if (!strcmp(info_key, "hostfile")) {
                status = HYDU_parse_hostfile(info_val, &pg->user_node_list,
                                             HYDU_process_mfile_token);
                HYDU_ERR_POP(status, "error parsing hostfile\n");
            }
            else {
                /* Unrecognized info key; ignore */
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
            HYD_STRING_STASH_INIT(stash);
            HYD_STRING_STASH(stash, HYDU_strdup(path), status);
            HYD_STRING_STASH(stash, HYDU_strdup("/"), status);
            HYD_STRING_STASH(stash, HYDU_strdup(val), status);

            HYD_STRING_SPIT(stash, execname, status);
        }

        i = 0;
        exec->exec[i++] = execname;
        for (k = 0; k < argcnt; k++) {
            HYDU_snprintf(key, PMI_MAXKEYLEN, "argv%d", k);
            val = HYD_pmcd_pmi_find_token_keyval(&tokens[segment_list[j].start_idx],
                                                 segment_list[j].token_count, key);
            HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                                "unable to find token: %s\n", key);
            exec->exec[i++] = HYDU_strdup(val);
        }
        exec->exec[i++] = NULL;

        exec->proc_count = maxprocs;

        /* It is not clear what kind of environment needs to get
         * passed to the spawned process. Don't set anything here, and
         * let the proxy do whatever it does by default. */
        exec->env_prop = NULL;

        status = HYDU_env_create(&env, "PMI_SPAWNED", "1");
        HYDU_ERR_POP(status, "unable to create PMI_SPAWNED environment\n");
        exec->user_env = env;
    }

    status = HYD_pmcd_pmi_alloc_pg_scratch(pg);
    HYDU_ERR_POP(status, "unable to allocate pg scratch space\n");

    pg->pg_process_count = 0;
    for (exec = exec_list; exec; exec = exec->next)
        pg->pg_process_count += exec->proc_count;

    pg_scratch = (struct HYD_pmcd_pmi_pg_scratch *) pg->pg_scratch;

    /* Get the common keys and deal with them */
    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "preputcount");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                        "unable to find token: preputcount\n");
    preputcount = atoi(val);

    for (i = 0; i < preputcount; i++) {
        char *preput_key, *preput_val;

        HYDU_snprintf(key, PMI_MAXKEYLEN, "ppkey%d", i);
        val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_key = val;

        HYDU_snprintf(key, PMI_MAXKEYLEN, "ppval%d", i);
        val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, key);
        HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR,
                            "unable to find token: %s\n", key);
        preput_val = val;

        status = HYD_pmcd_pmi_add_kvs(preput_key, preput_val, pg_scratch->kvs, &ret);
        HYDU_ERR_POP(status, "unable to add keypair to kvs\n");
    }

    /* Create the proxy list */
    if (pg->user_node_list) {
        status = HYDU_create_proxy_list(exec_list, pg->user_node_list, pg);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    }
    else {
        status = HYDU_create_proxy_list(exec_list, HYD_server_info.node_list, pg);
        HYDU_ERR_POP(status, "error creating proxy list\n");
    }
    HYDU_free_exec_list(exec_list);

    if (pg->user_node_list) {
        pg->pg_core_count = 0;
        for (i = 0, node = pg->user_node_list; node; node = node->next, i++)
            pg->pg_core_count += node->core_count;
    }
    else {
        pg->pg_core_count = 0;
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            pg->pg_core_count += proxy->node->core_count;
    }

    status = HYDU_sock_create_and_listen_portstr(HYD_server_info.user_global.iface,
                                                 HYD_server_info.localhost,
                                                 HYD_server_info.port_range, &control_port,
                                                 HYD_pmcd_pmiserv_control_listen_cb,
                                                 (void *) (size_t) new_pgid);
    HYDU_ERR_POP(status, "unable to create PMI port\n");
    if (HYD_server_info.user_global.debug)
        HYDU_dump(stdout, "Got a control port string of %s\n", control_port);

    /* Go to the last PG */
    for (pg = &HYD_server_info.pg_list; pg->next; pg = pg->next);

    status = HYD_pmcd_pmi_fill_in_proxy_args(&proxy_stash, control_port, new_pgid);
    HYDU_ERR_POP(status, "unable to fill in proxy arguments\n");
    HYDU_FREE(control_port);

    status = HYD_pmcd_pmi_fill_in_exec_launch_info(pg);
    HYDU_ERR_POP(status, "unable to fill in executable arguments\n");

    status = HYDT_bsci_launch_procs(proxy_stash.strlist, pg->proxy_list, HYD_FALSE, NULL);
    HYDU_ERR_POP(status, "launcher cannot launch processes\n");

    {
        char *cmd;

        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("cmd=spawn-response;"), status);
        if (thrid) {
            HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
            HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
            HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
        }
        HYD_STRING_STASH(stash, HYDU_strdup("rc=0;"), status);
        HYD_STRING_STASH(stash, HYDU_strdup("jobid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(pg_scratch->kvs->kvsname), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
        HYD_STRING_STASH(stash, HYDU_strdup("nerrs=0;"), status);

        HYD_STRING_SPIT(stash, cmd, status);

        status = cmd_response(fd, pid, cmd);
        HYDU_ERR_POP(status, "send command failed\n");
        HYDU_FREE(cmd);
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYD_STRING_STASH_FREE(proxy_stash);
    if (segment_list)
        HYDU_FREE(segment_list);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_publish(int fd, int pid, int pgid, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd, *thrid, *val, *name = NULL, *port = NULL;
    int token_count, success;
    struct HYD_pmcd_token *tokens = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");
    name = HYDU_strdup(val);

    if ((val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "port")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: port\n");
    port = HYDU_strdup(val);

    status = HYD_pmcd_pmi_publish(name, port, &success);
    HYDU_ERR_POP(status, "error publishing service\n");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=name-publish-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    if (!success) {
        HYD_STRING_STASH(stash, HYDU_strdup("rc=1;errmsg=duplicate_service_"), status);
        HYD_STRING_STASH(stash, HYDU_strdup(name), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    else
        HYD_STRING_STASH(stash, HYDU_strdup("rc=0;"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
    if (name)
        HYDU_FREE(name);
    if (port)
        HYDU_FREE(port);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_unpublish(int fd, int pid, int pgid, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd, *thrid, *name;
    int token_count, success;
    struct HYD_pmcd_token *tokens = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    if ((name = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");

    status = HYD_pmcd_pmi_unpublish(name, &success);
    HYDU_ERR_POP(status, "error unpublishing service\n");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=name-unpublish-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    if (success)
        HYD_STRING_STASH(stash, HYDU_strdup("rc=0;"), status);
    else {
        HYD_STRING_STASH(stash, HYDU_strdup("rc=1;errmsg=service_"), status);
        HYD_STRING_STASH(stash, HYDU_strdup(name), status);
        HYD_STRING_STASH(stash, HYDU_strdup("_not_found;"), status);
    }

    HYD_STRING_SPIT(stash, cmd, status);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_name_lookup(int fd, int pid, int pgid, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd, *thrid, *name, *value;
    int token_count;
    struct HYD_pmcd_token *tokens = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    thrid = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "thrid");

    if ((name = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "name")) == NULL)
        HYDU_ERR_POP(status, "cannot find token: name\n");

    status = HYD_pmcd_pmi_lookup(name, &value);
    HYDU_ERR_POP(status, "error while looking up service\n");

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=name-lookup-response;"), status);
    if (thrid) {
        HYD_STRING_STASH(stash, HYDU_strdup("thrid="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(thrid), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";"), status);
    }
    if (value) {
        HYD_STRING_STASH(stash, HYDU_strdup("port="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(value), status);
        HYD_STRING_STASH(stash, HYDU_strdup(";found=TRUE;rc=0;"), status);
    }
    else {
        HYD_STRING_STASH(stash, HYDU_strdup("found=FALSE;rc=1;"), status);
    }

    HYD_STRING_SPIT(stash, cmd, status);

    status = cmd_response(fd, pid, cmd);
    HYDU_ERR_POP(status, "send command failed\n");
    HYDU_FREE(cmd);

  fn_exit:
    if (tokens)
        HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/* TODO: abort, create_kvs, destroy_kvs, getbyidx */
static struct HYD_pmcd_pmi_handle pmi_v2_handle_fns_foo[] = {
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
