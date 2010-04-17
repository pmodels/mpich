/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmiserv_pmi.h"
#include "pmiserv_utils.h"

struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle = { 0 };
struct HYD_pmcd_pmi_publish *HYD_pmcd_pmi_publish_list = NULL;

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
    }

    proxy_scratch = (struct HYD_pmcd_pmi_proxy_scratch *) proxy->proxy_scratch;

    /* Add process to the node */
    HYDU_MALLOC(process, struct HYD_pmcd_pmi_process *, sizeof(struct HYD_pmcd_pmi_process),
                status);
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
    int do_break;

    do_break = 0;
    for (pg = &HYD_handle.pg_list; pg; pg = pg->next) {
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next) {
            if (proxy->control_fd == fd) {
                do_break = 1;
                break;
            }
        }
        if (do_break)
            break;
    }

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

HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmcd_pmi_free_publish(struct HYD_pmcd_pmi_publish *publish)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FREE(publish->name);
    HYDU_FREE(publish->port);

    for (i = 0; i < publish->infokeycount; i++) {
        HYDU_FREE(publish->info_keys[i].key);
        HYDU_FREE(publish->info_keys[i].val);
    }
    HYDU_FREE(publish->info_keys);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
