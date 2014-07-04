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
#include "hydt_ftb.h"

#define debug(...)                              \
    {                                           \
        if (HYD_pmcd_pmip.user_global.debug)    \
            HYDU_dump(stdout, __VA_ARGS__);     \
    }

/* The number of key values we store has to be one less than the
 * number of PMI arguments allowed.  This is because, when we flush
 * the PMI keyvals upstream, the server will treat it as a single PMI
 * command.  We leave room for one extra argument, cmd=put, that needs
 * to be appended when flushing. */
#define CACHE_PUT_KEYVAL_MAXLEN  (MAX_PMI_ARGS - 1)

static struct {
    char *keyval[CACHE_PUT_KEYVAL_MAXLEN + 1];
    int keyval_len;
} cache_put;

static struct {
    char **key;
    char **val;
    int keyval_len;
} cache_get;

static HYD_status send_cmd_upstream(const char *start, int fd, int num_args, char *args[])
{
    int i, j, sent, closed;
    char **tmp, *buf;
    struct HYD_pmcd_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* We need two slots for each argument (one for the argument
     * itself and one for a space character), one slot for the
     * command, and one for the NULL character at the end. */
    HYDU_MALLOC(tmp, char **, (2 * num_args + 2) * sizeof(char *), status);

    j = 0;
    tmp[j++] = HYDU_strdup(start);
    for (i = 0; i < num_args; i++) {
        tmp[j++] = HYDU_strdup(args[i]);
        if (args[i + 1])
            tmp[j++] = HYDU_strdup(" ");
    }
    tmp[j] = NULL;

    status = HYDU_str_alloc_and_join(tmp, &buf);
    HYDU_ERR_POP(status, "unable to join strings\n");
    HYDU_free_strlist(tmp);
    HYDU_FREE(tmp);

    HYD_pmcd_init_header(&hdr);
    hdr.cmd = PMI_CMD;
    hdr.pid = fd;
    hdr.buflen = strlen(buf);
    hdr.pmi_version = 1;
    status =
        HYDU_sock_write(HYD_pmcd_pmip.upstream.control, &hdr, sizeof(hdr), &sent, &closed,
                        HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI header upstream\n");
    HYDU_ASSERT(!closed, status);

    debug("forwarding command (%s) upstream\n", buf);

    status = HYDU_sock_write(HYD_pmcd_pmip.upstream.control, buf, hdr.buflen, &sent, &closed,
                             HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "unable to send PMI command upstream\n");
    HYDU_ASSERT(!closed, status);

    HYDU_FREE(buf);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status send_cmd_downstream(int fd, const char *cmd)
{
    int sent, closed;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    debug("PMI response: %s", cmd);

    status = HYDU_sock_write(fd, cmd, strlen(cmd), &sent, &closed, HYDU_SOCK_COMM_MSGWAIT);
    HYDU_ERR_POP(status, "error writing PMI line\n");
    /* FIXME: We cannot abort when we are not able to send data
     * downstream. The upper layer needs to handle this based on
     * whether we want to abort or not.*/
    HYDU_ASSERT(!closed, status);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status cache_put_flush(int fd)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (cache_put.keyval_len == 0)
        goto fn_exit;

    debug("flushing %d put command(s) out\n", cache_put.keyval_len);

    status = send_cmd_upstream("cmd=put ", fd, cache_put.keyval_len, cache_put.keyval);
    HYDU_ERR_POP(status, "error sending command upstream\n");

    for (i = 0; i < cache_put.keyval_len; i++)
        HYDU_FREE(cache_put.keyval[i]);
    cache_put.keyval_len = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_init(int fd, char *args[])
{
    int pmi_version, pmi_subversion, i;
    const char *tmp = NULL;
    static int global_init = 1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    strtok(args[0], "=");
    pmi_version = atoi(strtok(NULL, "="));
    strtok(args[1], "=");
    pmi_subversion = atoi(strtok(NULL, "="));

    if (pmi_version == 1 && pmi_subversion <= 1)
        tmp = HYDU_strdup("cmd=response_to_init pmi_version=1 pmi_subversion=1 rc=0\n");
    else if (pmi_version == 2 && pmi_subversion == 0)
        tmp = HYDU_strdup("cmd=response_to_init pmi_version=2 pmi_subversion=0 rc=0\n");
    else        /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);

    status = send_cmd_downstream(fd, tmp);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(tmp);

    /* initialize some structures; these are initialized exactly once,
     * even if the init command is sent once from each process. */
    if (global_init) {
        for (i = 0; i < CACHE_PUT_KEYVAL_MAXLEN + 1; i++)
            cache_put.keyval[i] = NULL;
        cache_put.keyval_len = 0;
        cache_get.keyval_len = 0;
        global_init = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_initack(int fd, char *args[])
{
    int id, i;
    char *val, *cmd;
    struct HYD_pmcd_token *tokens;
    int token_count;
    struct HYD_string_stash stash;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "pmiid");
    HYDU_ERR_CHKANDJUMP(status, val == NULL, HYD_INTERNAL_ERROR, "unable to find pmiid token\n");
    id = atoi(val);

    /* Store the PMI_ID to fd mapping */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        if (HYD_pmcd_pmip.downstream.pmi_rank[i] == id) {
            HYD_pmcd_pmip.downstream.pmi_fd[i] = fd;
            HYD_pmcd_pmip.downstream.pmi_fd_active[i] = 1;
            break;
        }
    }
    HYDU_ASSERT(i < HYD_pmcd_pmip.local.proxy_process_count, status);

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=initack\ncmd=set size="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_process_count),
                     status);

    HYD_STRING_STASH(stash, HYDU_strdup("\ncmd=set rank="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(id), status);

    HYD_STRING_STASH(stash, HYDU_strdup("\ncmd=set debug="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(HYD_pmcd_pmip.user_global.debug), status);
    HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_maxes(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=maxes kvsname_max="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(PMI_MAXKVSLEN), status);
    HYD_STRING_STASH(stash, HYDU_strdup(" keylen_max="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(PMI_MAXKEYLEN), status);
    HYD_STRING_STASH(stash, HYDU_strdup(" vallen_max="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(PMI_MAXVALLEN), status);
    HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_appnum(int fd, char *args[])
{
    int i, idx;
    struct HYD_exec *exec;
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Get the process index */
    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++)
        if (HYD_pmcd_pmip.downstream.pmi_fd[i] == fd)
            break;
    idx = i;
    HYDU_ASSERT(idx < HYD_pmcd_pmip.local.proxy_process_count, status);

    i = 0;
    for (exec = HYD_pmcd_pmip.exec_list; exec; exec = exec->next) {
        i += exec->proc_count;
        if (idx < i)
            break;
    }

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=appnum appnum="), status);
    HYD_STRING_STASH(stash, HYDU_int_to_str(exec->appnum), status);
    HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_my_kvsname(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=my_kvsname kvsname="), status);
    HYD_STRING_STASH(stash, HYDU_strdup(HYD_pmcd_pmip.local.kvs->kvsname), status);
    HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_usize(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup("cmd=universe_size size="), status);
    if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_SYSTEM)
        HYD_STRING_STASH(stash,
                         HYDU_int_to_str(HYD_pmcd_pmip.system_global.global_core_map.global_count),
                         status);
    else if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_INFINITE)
        HYD_STRING_STASH(stash, HYDU_int_to_str(-1), status);
    else
        HYD_STRING_STASH(stash, HYDU_int_to_str(HYD_pmcd_pmip.user_global.usize), status);
    HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

    HYD_STRING_SPIT(stash, cmd, status);

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd, *key, *val;
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    if (!strcmp(key, "PMI_process_mapping")) {
        HYD_STRING_STASH_INIT(stash);
        HYD_STRING_STASH(stash, HYDU_strdup("cmd=get_result rc=0 msg=success value="), status);
        HYD_STRING_STASH(stash, HYDU_strdup(HYD_pmcd_pmip.system_global.pmi_process_mapping),
                         status);
        HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

        HYD_STRING_SPIT(stash, cmd, status);

        status = send_cmd_downstream(fd, cmd);
        HYDU_ERR_POP(status, "error sending PMI response\n");
        HYDU_FREE(cmd);
    }
    else {
        val = NULL;
        for (i = 0; i < cache_get.keyval_len; i++) {
            if (!strcmp(cache_get.key[i], key)) {
                val = cache_get.val[i];
                break;
            }
        }

        if (val) {
            HYD_STRING_STASH_INIT(stash);
            HYD_STRING_STASH(stash, HYDU_strdup("cmd=get_result rc="), status);
            HYD_STRING_STASH(stash, HYDU_strdup("0 msg=success value="), status);
            HYD_STRING_STASH(stash, HYDU_strdup(val), status);
            HYD_STRING_STASH(stash, HYDU_strdup("\n"), status);

            HYD_STRING_SPIT(stash, cmd, status);

            status = send_cmd_downstream(fd, cmd);
            HYDU_ERR_POP(status, "error sending PMI response\n");
            HYDU_FREE(cmd);
        }
        else {
            /* if we can't find the key locally, ask upstream */
            status = send_cmd_upstream("cmd=get ", fd, token_count, args);
            HYDU_ERR_POP(status, "error sending command upstream\n");
        }
    }

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_put(int fd, char *args[])
{
    struct HYD_string_stash stash;
    char *cmd;
    char *key, *val;
    struct HYD_pmcd_token *tokens;
    int token_count;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    key = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    val = HYD_pmcd_pmi_find_token_keyval(tokens, token_count, "value");
    if (val == NULL)
        val = HYDU_strdup("");

    /* add to the cache */
    HYD_STRING_STASH_INIT(stash);
    HYD_STRING_STASH(stash, HYDU_strdup(key), status);
    HYD_STRING_STASH(stash, HYDU_strdup("="), status);
    HYD_STRING_STASH(stash, HYDU_strdup(val), status);

    HYD_STRING_SPIT(stash, cmd, status);

    cache_put.keyval[cache_put.keyval_len++] = cmd;
    debug("cached command: %s\n", cmd);

    if (cache_put.keyval_len >= CACHE_PUT_KEYVAL_MAXLEN)
        cache_put_flush(fd);

    status = send_cmd_downstream(fd, "cmd=put_result rc=0 msg=success\n");
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_keyval_cache(int fd, char *args[])
{
    struct HYD_pmcd_token *tokens;
    int token_count, i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_pmi_args_to_tokens(args, &tokens, &token_count);
    HYDU_ERR_POP(status, "unable to convert args to tokens\n");

    /* allocate a larger space for the cached keyvals, copy over the
     * older keyvals and add the new ones in */
    HYDU_REALLOC(cache_get.key, char **, (cache_get.keyval_len + token_count) * sizeof(char *),
                 status);
    HYDU_REALLOC(cache_get.val, char **, (cache_get.keyval_len + token_count) * sizeof(char *),
                 status);

    for (i = 0; i < token_count; i++) {
        cache_get.key[cache_get.keyval_len + i] = HYDU_strdup(tokens[i].key);
        cache_get.val[cache_get.keyval_len + i] = HYDU_strdup(tokens[i].val);
    }
    cache_get.keyval_len += token_count;

  fn_exit:
    HYD_pmcd_pmi_free_tokens(tokens, token_count);
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_in(int fd, char *args[])
{
    static int barrier_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    barrier_count++;
    if (barrier_count == HYD_pmcd_pmip.local.proxy_process_count) {
        barrier_count = 0;

        cache_put_flush(fd);

        status = send_cmd_upstream("cmd=barrier_in", fd, 0, args);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_out(int fd, char *args[])
{
    const char *cmd;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = HYDU_strdup("cmd=barrier_out\n");

    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        status = send_cmd_downstream(HYD_pmcd_pmip.downstream.pmi_fd[i], cmd);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    }

    HYDU_FREE(cmd);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_finalize(int fd, char *args[])
{
    const char *cmd;
    int i;
    static int finalize_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    cmd = HYDU_strdup("cmd=finalize_ack\n");

    status = send_cmd_downstream(fd, cmd);
    HYDU_ERR_POP(status, "error sending PMI response\n");
    HYDU_FREE(cmd);

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

    finalize_count++;

    if (finalize_count == HYD_pmcd_pmip.local.proxy_process_count) {
        /* All processes have finalized */
        for (i = 0; i < cache_get.keyval_len; i++) {
            HYDU_FREE(cache_get.key[i]);
            HYDU_FREE(cache_get.val[i]);
        }
        HYDU_FREE(cache_get.key);
        HYDU_FREE(cache_get.val);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static struct HYD_pmcd_pmip_pmi_handle pmi_v1_handle_fns_foo[] = {
    {"init", fn_init},
    {"initack", fn_initack},
    {"get_maxes", fn_get_maxes},
    {"get_appnum", fn_get_appnum},
    {"get_my_kvsname", fn_get_my_kvsname},
    {"get_universe_size", fn_get_usize},
    {"get", fn_get},
    {"put", fn_put},
    {"keyval_cache", fn_keyval_cache},
    {"barrier_in", fn_barrier_in},
    {"barrier_out", fn_barrier_out},
    {"finalize", fn_finalize},
    {"\0", NULL}
};

struct HYD_pmcd_pmip_pmi_handle *HYD_pmcd_pmip_pmi_v1 = pmi_v1_handle_fns_foo;
