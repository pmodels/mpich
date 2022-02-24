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

/* The number of key values we store has to be one less than the
 * number of PMI arguments allowed.  This is because, when we flush
 * the PMI keyvals upstream, the server will treat it as a single PMI
 * command.  We leave room for one extra argument, cmd=put, that needs
 * to be appended when flushing. */
#define CACHE_PUT_KEYVAL_MAXLEN  (MAX_PMI_ARGS - 1)

static struct {
    struct PMIU_token tokens[CACHE_PUT_KEYVAL_MAXLEN + 1];
    int keyval_len;
} cache_put;

struct cache_elem {
    char *key;
    char *val;
    UT_hash_handle hh;
};

static struct cache_elem *cache_get = NULL, *hash_get = NULL;

static int num_elems = 0;

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

static HYD_status cache_put_flush(int fd)
{
    HYD_status status = HYD_SUCCESS;
    HYDU_FUNC_ENTER();

    if (cache_put.keyval_len == 0)
        goto fn_exit;

    debug("flushing %d put command(s) out\n", cache_put.keyval_len);

    struct PMIU_cmd pmi;
    PMIU_cmd_init(&pmi, 1, "put");
    HYDU_ASSERT(pmi.num_tokens < MAX_PMI_ARGS, status);
    for (int i = 0; i < cache_put.keyval_len; i++) {
        PMIU_cmd_add_str(&pmi, cache_put.tokens[i].key, cache_put.tokens[i].val);
    }

    if (HYD_pmcd_pmip.user_global.debug) {
        HYDU_dump(stdout, "forwarding command upstream:\n");
        HYD_pmcd_pmi_dump(&pmi);
    }
    status = send_cmd_upstream(&pmi, fd);
    HYDU_ERR_POP(status, "error sending command upstream\n");

    for (int i = 0; i < cache_put.keyval_len; i++) {
        MPL_free(cache_put.tokens[i].key);
        MPL_free(cache_put.tokens[i].val);
    }
    cache_put.keyval_len = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_init(int fd, struct PMIU_cmd *pmi)
{
    int pmi_version, pmi_subversion;
    const char *tmp = NULL;
    static int global_init = 1;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    tmp = PMIU_cmd_find_keyval(pmi, "pmi_version");
    HYDU_ERR_CHKANDJUMP(status, tmp == NULL, HYD_INTERNAL_ERROR,
                        "unable to find pmi_version token\n");
    pmi_version = atoi(tmp);

    tmp = PMIU_cmd_find_keyval(pmi, "pmi_subversion");
    HYDU_ERR_CHKANDJUMP(status, tmp == NULL, HYD_INTERNAL_ERROR,
                        "unable to find pmi_subversion token\n");
    pmi_subversion = atoi(tmp);

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "response_to_init");
    if (pmi_version == 1 && pmi_subversion <= 1) {
        PMIU_cmd_add_str(&pmi_response, "pmi_version", "1");
        PMIU_cmd_add_str(&pmi_response, "pmi_subversion", "1");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else if (pmi_version == 2 && pmi_subversion == 0) {
        PMIU_cmd_add_str(&pmi_response, "pmi_version", "2");
        PMIU_cmd_add_str(&pmi_response, "pmi_subversion", "0");
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
    } else {    /* PMI version mismatch */
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR,
                            "PMI version mismatch; %d.%d\n", pmi_version, pmi_subversion);
    }

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    /* initialize some structures; these are initialized exactly once,
     * even if the init command is sent once from each process. */
    if (global_init) {
        cache_put.keyval_len = 0;
        global_init = 0;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_initack(int fd, struct PMIU_cmd *pmi)
{
    int id, i;
    const char *val;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    val = PMIU_cmd_find_keyval(pmi, "pmiid");
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

    struct PMIU_cmd pmi_response;

    PMIU_cmd_init(&pmi_response, 1, "initack");
    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    int size = HYD_pmcd_pmip.system_global.global_process_count;
    int rank = id;
    int debug = HYD_pmcd_pmip.user_global.debug;
    PMIU_cmd_init(&pmi_response, 1, "set");
    PMIU_cmd_add_int(&pmi_response, "size", size);
    PMIU_cmd_add_int(&pmi_response, "rank", rank);
    PMIU_cmd_add_int(&pmi_response, "debug", debug);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_maxes(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "maxes");
    PMIU_cmd_add_int(&pmi_response, "kvsname_max", PMI_MAXKVSLEN);
    PMIU_cmd_add_int(&pmi_response, "keylen_max", PMI_MAXKEYLEN);
    PMIU_cmd_add_int(&pmi_response, "vallen_max", PMI_MAXVALLEN);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_appnum(int fd, struct PMIU_cmd *pmi)
{
    int i, idx;
    struct HYD_exec *exec;
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

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "appnum");
    PMIU_cmd_add_int(&pmi_response, "appnum", exec->appnum);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_my_kvsname(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "my_kvsname");
    PMIU_cmd_add_str(&pmi_response, "kvsname", HYD_pmcd_pmip.local.kvs->kvsname);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get_usize(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    int universe_size;
    if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_SYSTEM) {
        universe_size = HYD_pmcd_pmip.system_global.global_core_map.global_count;
    } else if (HYD_pmcd_pmip.user_global.usize == HYD_USIZE_INFINITE) {
        universe_size = -1;
    } else {
        universe_size = HYD_pmcd_pmip.user_global.usize;
    }

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "universe_size");
    PMIU_cmd_add_int(&pmi_response, "size", universe_size);

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_get(int fd, struct PMIU_cmd *pmi)
{
    const char *key;
    struct cache_elem *found = NULL;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "get_result");

    if (!strcmp(key, "PMI_process_mapping")) {
        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
        PMIU_cmd_add_str(&pmi_response, "value", HYD_pmcd_pmip.system_global.pmi_process_mapping);

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    } else if (!strcmp(key, "PMI_hwloc_xmlfile")) {
        const char *xmlfile = HYD_pmip_get_hwloc_xmlfile();

        PMIU_cmd_add_str(&pmi_response, "rc", "0");
        PMIU_cmd_add_str(&pmi_response, "msg", "success");
        if (xmlfile) {
            PMIU_cmd_add_str(&pmi_response, "value", xmlfile);
        } else {
            PMIU_cmd_add_str(&pmi_response, "value", "unavailable");
        }

        status = send_cmd_downstream(fd, &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    } else {
        HASH_FIND_STR(hash_get, key, found);
        if (found) {
            PMIU_cmd_add_str(&pmi_response, "rc", "0");
            PMIU_cmd_add_str(&pmi_response, "msg", "success");
            PMIU_cmd_add_str(&pmi_response, "value", found->val);

            status = send_cmd_downstream(fd, &pmi_response);
            HYDU_ERR_POP(status, "error sending PMI response\n");
        } else {
            /* if we can't find the key locally, ask upstream */
            status = send_cmd_upstream(pmi, fd);
        }
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_put(int fd, struct PMIU_cmd *pmi)
{
    const char *key, *val;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key = PMIU_cmd_find_keyval(pmi, "key");
    HYDU_ERR_CHKANDJUMP(status, key == NULL, HYD_INTERNAL_ERROR, "unable to find token: key\n");

    val = PMIU_cmd_find_keyval(pmi, "value");
    if (val == NULL)
        val = MPL_strdup("");
    HYDU_ERR_CHKANDJUMP(status, NULL == val, HYD_INTERNAL_ERROR, "strdup failed\n");

    /* add to the cache */
    int i = cache_put.keyval_len++;
    cache_put.tokens[i].key = MPL_strdup(key);
    cache_put.tokens[i].val = MPL_strdup(val);
    debug("cached command: %s=%s\n", key, val);

    if (cache_put.keyval_len >= CACHE_PUT_KEYVAL_MAXLEN)
        cache_put_flush(fd);

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "put_result");
    PMIU_cmd_add_str(&pmi_response, "rc", "0");
    PMIU_cmd_add_str(&pmi_response, "msg", "success");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_keyval_cache(int fd, struct PMIU_cmd *pmi)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* FIXME: leak of abstraction of the pmi object */

    /* allocate a larger space for the cached keyvals, copy over the
     * older keyvals and add the new ones in */
    HASH_CLEAR(hh, hash_get);
    HYDU_REALLOC_OR_JUMP(cache_get, struct cache_elem *,
                         (sizeof(struct cache_elem) * (num_elems + pmi->num_tokens)), status);

    int i;
    for (i = 0; i < num_elems; i++) {
        struct cache_elem *elem = cache_get + i;
        HASH_ADD_STR(hash_get, key, elem, MPL_MEM_PM);
    }
    for (; i < num_elems + pmi->num_tokens; i++) {
        struct cache_elem *elem = cache_get + i;
        elem->key = MPL_strdup(pmi->tokens[i - num_elems].key);
        HYDU_ERR_CHKANDJUMP(status, NULL == elem->key, HYD_INTERNAL_ERROR, "%s", "");
        elem->val = MPL_strdup(pmi->tokens[i - num_elems].val);
        HYDU_ERR_CHKANDJUMP(status, NULL == elem->val, HYD_INTERNAL_ERROR, "%s", "");
        HASH_ADD_STR(hash_get, key, elem, MPL_MEM_PM);
    }
    num_elems += pmi->num_tokens;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_in(int fd, struct PMIU_cmd *pmi)
{
    static int barrier_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    barrier_count++;
    if (barrier_count == HYD_pmcd_pmip.local.proxy_process_count) {
        barrier_count = 0;

        cache_put_flush(fd);

        status = send_cmd_upstream(pmi, fd);
        HYDU_ERR_POP(status, "error sending command upstream\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_barrier_out(int fd, struct PMIU_cmd *pmi)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "barrier_out");

    for (i = 0; i < HYD_pmcd_pmip.local.proxy_process_count; i++) {
        status = send_cmd_downstream(HYD_pmcd_pmip.downstream.pmi_fd[i], &pmi_response);
        HYDU_ERR_POP(status, "error sending PMI response\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

static HYD_status fn_finalize(int fd, struct PMIU_cmd *pmi)
{
    int i;
    static int finalize_count = 0;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    struct PMIU_cmd pmi_response;
    PMIU_cmd_init(&pmi_response, 1, "finalize_ack");

    status = send_cmd_downstream(fd, &pmi_response);
    HYDU_ERR_POP(status, "error sending PMI response\n");

    status = HYDT_dmx_deregister_fd(fd);
    HYDU_ERR_POP(status, "unable to deregister fd\n");
    close(fd);

    finalize_count++;

    if (finalize_count == HYD_pmcd_pmip.local.proxy_process_count) {
        /* All processes have finalized */
        HASH_CLEAR(hh, hash_get);
        for (i = 0; i < num_elems; i++) {
            MPL_free((cache_get + i)->key);
            MPL_free((cache_get + i)->val);
        }
        MPL_free(cache_get);
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
