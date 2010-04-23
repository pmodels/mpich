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

struct HYD_proxy *HYD_pmcd_pmi_find_proxy(int fd)
{
    struct HYD_pg *pg;
    struct HYD_proxy *proxy;

    for (pg = &HYD_handle.pg_list; pg; pg = pg->next)
        for (proxy = pg->proxy_list; proxy; proxy = proxy->next)
            if (proxy->control_fd == fd)
                return proxy;

    return NULL;
}

HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_FUNC_EXIT();
    return status;
}

HYD_status HYD_pmcd_pmi_free_publish(struct HYD_pmcd_pmi_publish * publish)
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
    if (publish->info_keys)
        HYDU_FREE(publish->info_keys);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
