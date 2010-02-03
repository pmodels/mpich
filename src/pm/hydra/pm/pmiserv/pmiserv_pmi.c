/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle = { 0 };

HYD_status HYD_pmcd_pmi_id_to_rank(int id, int pgid, int *rank)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_pmcd_pmi_process *process;
    int max, ll, ul;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Our rank should be >= (id * ranks_per_proc) and < ((id + 1) *
     * ranks_per_proc. Find the maximum value available to use */
    for (pg = &HYD_handle.pg_list; pg->pgid != pgid; pg = pg->next);
    if (!pg)
        HYDU_ERR_SETANDJUMP1(status, HYD_INTERNAL_ERROR, "PMI pgid %d not found\n", pgid);

    ll = id * HYD_handle.ranks_per_proc;
    ul = ((id + 1) * HYD_handle.ranks_per_proc) - 1;

    max = ll;
    for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
        proxy_scratch = proxy->proxy_scratch;

        if (proxy_scratch == NULL)
            break;

        for (process = proxy_scratch->process_list; process; process = process->next) {
            if (max <= process->rank && process->rank <= ul)
                max++;
        }
    }
    *rank = max;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

void HYD_pmcd_pmi_segment_tokens(struct HYD_pmcd_token *tokens, int token_count,
                                 struct HYD_pmcd_token_segment *segment_list,
                                 int *num_segments)
{
    int i, j;

    j = 0;
    segment_list[j].start_idx = 0;
    segment_list[j].token_count = 0;
    for (i = 0; i < token_count; i++) {
        if (!strcmp(tokens[i].key, "endcmd") && (i < token_count - 1)) {
            j++;
            segment_list[j].start_idx = i + 1;
            segment_list[j].token_count = 0;
        }
        else {
            segment_list[j].token_count++;
        }
    }
    *num_segments = j + 1;
}

HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, struct HYD_pmcd_pmi_kvs * kvs,
                                int *ret)
{
    struct HYD_pmcd_pmi_kvs_pair *key_pair, *run;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(key_pair, struct HYD_pmcd_pmi_kvs_pair *, sizeof(struct HYD_pmcd_pmi_kvs_pair),
                status);
    HYDU_snprintf(key_pair->key, MAXKEYLEN, "%s", key);
    HYDU_snprintf(key_pair->val, MAXVALLEN, "%s", val);
    key_pair->next = NULL;

    *ret = 0;

    if (kvs->key_pair == NULL) {
        kvs->key_pair = key_pair;
    }
    else {
        run = kvs->key_pair;
        while (run->next) {
            if (!strcmp(run->key, key_pair->key)) {
                /* duplicate key found */
                *ret = -1;
                break;
            }
            run = run->next;
        }
        run->next = key_pair;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_process_mapping(char **process_mapping_str)
{
    int i, node_id;
    char *tmp[HYD_NUM_TMP_STRINGS];
    struct HYD_proxy *proxy;
    struct block {
        int num_blocks;
        int block_size;
        struct block *next;
    } *blocklist_head, *blocklist_tail = NULL, *block, *nblock;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    blocklist_head = NULL;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        if (blocklist_head == NULL) {
            HYDU_MALLOC(blocklist_head, struct block *, sizeof(struct block), status);
            blocklist_head->block_size = proxy->node.core_count;
            blocklist_head->num_blocks = 1;
            blocklist_head->next = NULL;
            blocklist_tail = blocklist_head;
        }
        else if (blocklist_tail->block_size == proxy->node.core_count) {
            blocklist_tail->num_blocks++;
        }
        else {
            HYDU_MALLOC(blocklist_tail->next, struct block *, sizeof(struct block), status);
            blocklist_tail = blocklist_tail->next;
            blocklist_tail->block_size = proxy->node.core_count;
            blocklist_tail->num_blocks = 1;
            blocklist_tail->next = NULL;
        }
    }

    i = 0;
    tmp[i++] = HYDU_strdup("(");
    tmp[i++] = HYDU_strdup("vector,");
    node_id = 0;
    for (block = blocklist_head; block; block = block->next) {
        tmp[i++] = HYDU_strdup("(");
        tmp[i++] = HYDU_int_to_str(node_id++);
        tmp[i++] = HYDU_strdup(",");
        tmp[i++] = HYDU_int_to_str(block->num_blocks);
        tmp[i++] = HYDU_strdup(",");
        tmp[i++] = HYDU_int_to_str(block->block_size);
        tmp[i++] = HYDU_strdup(")");
        if (block->next)
            tmp[i++] = HYDU_strdup(",");
        HYDU_STRLIST_CONSOLIDATE(tmp, i, status);
    }
    tmp[i++] = HYDU_strdup(")");
    tmp[i++] = NULL;

    status = HYDU_str_alloc_and_join(tmp, process_mapping_str);
    HYDU_ERR_POP(status, "error while joining strings\n");

    HYDU_free_strlist(tmp);

    for (block = blocklist_head; block; block = nblock) {
        nblock = block->next;
        HYDU_FREE(block);
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_add_process_to_pg(struct HYD_pg *pg, int fd, int pid, int rank)
{
    struct HYD_pmcd_pmi_process *process, *tmp;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    int srank;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    srank = rank % HYD_handle.global_core_count;

    for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
        if ((srank >= proxy->start_pid) &&
            (srank < (proxy->start_pid + proxy->node.core_count)))
            break;

    if (proxy->proxy_scratch == NULL) {
        HYDU_MALLOC(proxy->proxy_scratch, void *, sizeof(struct HYD_pmcd_pmi_proxy_scratch),
                    status);

        proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;
        proxy_scratch->process_list = NULL;
        proxy_scratch->kvs = NULL;
    }

    proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;

    if (proxy_scratch->kvs == NULL) {
        status = HYD_pmcd_pmi_allocate_kvs(&proxy_scratch->kvs, pg->pgid);
        HYDU_ERR_POP(status, "unable to allocate kvs space\n");
    }

    /* Add process to the node */
    HYDU_MALLOC(process, struct HYD_pmcd_pmi_process *, sizeof(struct HYD_pmcd_pmi_process),
                status);
    process->fd = fd;
    process->pid = pid;
    process->rank = rank;
    process->epoch = 0;
    process->proxy = proxy;
    process->next = NULL;

    if (proxy_scratch->process_list == NULL)
        proxy_scratch->process_list = process;
    else {
        tmp = proxy_scratch->process_list;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = process;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

struct HYD_pmcd_pmi_process *HYD_pmcd_pmi_find_process(int fd, int pid)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_proxy_scratch *proxy_scratch;
    struct HYD_pmcd_pmi_process *process = NULL;

    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            if (proxy->proxy_scratch == NULL)
                continue;

            proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;
            for (process = proxy_scratch->process_list; process; process = process->next) {
                if (process->fd == fd && process->pid == pid)
                    return process;
            }
        }
    }

    return NULL;
}

HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmcd_pmi_v1_cmd_response(int fd, int pid, const char *cmd, int cmd_len,
                                        int finalize)
{
    enum HYD_pmcd_pmi_cmd c;
    struct HYD_pmcd_pmi_response_hdr hdr;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    c = PMI_RESPONSE;
    status = HYDU_sock_write(fd, &c, sizeof(c));
    HYDU_ERR_POP(status, "unable to send PMI_RESPONSE command to proxy\n");

    hdr.pid = pid;
    hdr.buflen = cmd_len;
    hdr.finalize = finalize;
    status = HYDU_sock_write(fd, &hdr, sizeof(hdr));
    HYDU_ERR_POP(status, "unable to send PMI_RESPONSE header to proxy\n");

    status = HYDU_sock_write(fd, cmd, cmd_len);
    HYDU_ERR_POP(status, "unable to send response to command\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
