/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmi_handle.h"

HYD_pmcd_pmi_pg_t *HYD_pg_list = NULL;
struct HYD_pmcd_pmi_handle *HYD_pmcd_pmi_handle = { 0 };

struct segment {
    int start_pid;
    int proc_count;
    int node_id;
    struct segment *next;
};

struct block {
    int start_node_id;
    int num_blocks;
    int block_size;
    struct block *next;
};

HYD_status HYD_pmcd_args_to_tokens(char *args[], struct HYD_pmcd_token **tokens, int *count)
{
    int i;
    char *arg;
    HYD_status status = HYD_SUCCESS;

    for (i = 0; args[i]; i++);
    *count = i;
    HYDU_MALLOC(*tokens, struct HYD_pmcd_token *, *count * sizeof(struct HYD_pmcd_token),
                status);

    for (i = 0; args[i]; i++) {
        arg = HYDU_strdup(args[i]);
        (*tokens)[i].key = strtok(arg, "=");
        (*tokens)[i].val = strtok(NULL, "=");
    }

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}

char *HYD_pmcd_find_token_keyval(struct HYD_pmcd_token *tokens, int count, const char *key)
{
    int i;

    for (i = 0; i < count; i++) {
        if (!strcmp(tokens[i].key, key))
            return tokens[i].val;
    }

    return NULL;
}

static HYD_status allocate_kvs(HYD_pmcd_pmi_kvs_t ** kvs, int pgid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*kvs, HYD_pmcd_pmi_kvs_t *, sizeof(HYD_pmcd_pmi_kvs_t), status);
    HYDU_snprintf((*kvs)->kvs_name, MAXNAMELEN, "kvs_%d_%d", (int) getpid(), pgid);
    (*kvs)->key_pair = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_create_pg(HYD_pmcd_pmi_pg_t ** pg, int pgid)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*pg, HYD_pmcd_pmi_pg_t *, sizeof(HYD_pmcd_pmi_pg_t), status);
    (*pg)->pgid = pgid;
    (*pg)->num_procs = 0;
    (*pg)->num_subgroups = 0;
    (*pg)->conn_procs = NULL;
    (*pg)->barrier_count = 0;
    (*pg)->node_list = NULL;

    status = allocate_kvs(&(*pg)->kvs, pgid);
    HYDU_ERR_POP(status, "unable to allocate kvs space\n");

    (*pg)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_status free_pmi_process_list(HYD_pmcd_pmi_process_t * process_list)
{
    HYD_pmcd_pmi_process_t *process, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    process = process_list;
    while (process) {
        tmp = process->next;
        HYDU_FREE(process);
        process = tmp;
    }

    HYDU_FUNC_EXIT();
    return status;
}


static HYD_status free_pmi_kvs_list(HYD_pmcd_pmi_kvs_t * kvs_list)
{
    HYD_pmcd_pmi_kvs_pair_t *key_pair, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    key_pair = kvs_list->key_pair;
    while (key_pair) {
        tmp = key_pair->next;
        HYDU_FREE(key_pair);
        key_pair = tmp;
    }
    HYDU_FREE(kvs_list);

    HYDU_FUNC_EXIT();
    return status;
}


static HYD_status free_pmi_node_list(HYD_pmcd_pmi_node_t * node_list)
{
    HYD_pmcd_pmi_node_t *node, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    node = node_list;
    while (node) {
        tmp = node->next;
        free_pmi_process_list(node->process_list);
        free_pmi_kvs_list(node->kvs);
        HYDU_FREE(node);
        node = tmp;
    }

    HYDU_FUNC_EXIT();
    return status;
}


static struct HYD_pmcd_pmi_node *allocate_node(HYD_pmcd_pmi_pg_t * pg, int node_id)
{
    struct HYD_pmcd_pmi_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(node, HYD_pmcd_pmi_node_t *, sizeof(HYD_pmcd_pmi_node_t), status);
    node->node_id = node_id;
    node->pg = pg;
    node->process_list = NULL;
    node->kvs = NULL;
    node->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return node;

  fn_fail:
    node = NULL;
    goto fn_exit;
}


HYD_status HYD_pmcd_pmi_add_kvs(const char *key, char *val, HYD_pmcd_pmi_kvs_t * kvs, int *ret)
{
    HYD_pmcd_pmi_kvs_pair_t *key_pair, *run;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(key_pair, HYD_pmcd_pmi_kvs_pair_t *, sizeof(HYD_pmcd_pmi_kvs_pair_t), status);
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


HYD_status HYD_pmcd_pmi_id_to_rank(int id, int *rank)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (HYD_handle.ranks_per_proc == -1) {
        /* If multiple procs per rank is not defined, use ID as the rank */
        *rank = id;
    }
    else {
        *rank = (id * HYD_handle.ranks_per_proc) + HYD_pg_list->conn_procs[id];
        HYD_pg_list->conn_procs[id]++;
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_status HYD_pmcd_pmi_process_mapping(HYD_pmcd_pmi_process_t * process,
                                        enum HYD_pmcd_pmi_process_mapping_type type,
                                        char **process_mapping_str)
{
    int i, node_id;
    char *tmp[HYD_NUM_TMP_STRINGS];
    struct HYD_proxy *proxy;
    struct segment *seglist_head, *seglist_tail = NULL, *seg, *nseg;
    struct block *blocklist_head, *blocklist_tail = NULL, *block, *nblock;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    seglist_head = NULL;
    node_id = -1;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        node_id++;

        HYDU_MALLOC(seg, struct segment *, sizeof(struct segment), status);
        seg->start_pid = proxy->start_pid;
        seg->proc_count = proxy->node.core_count;
        seg->node_id = node_id;
        seg->next = NULL;

        if (seglist_head == NULL) {
            seglist_head = seg;
            seglist_tail = seg;
        }
        else {
            seglist_tail->next = seg;
            seglist_tail = seg;
        }
    }

    /* Create a block list off the segment list */
    blocklist_head = NULL;
    for (seg = seglist_head; seg; seg = seg->next) {
        if (blocklist_head == NULL) {
            HYDU_MALLOC(blocklist_head, struct block *, sizeof(struct block), status);
            blocklist_head->start_node_id = seg->node_id;
            blocklist_head->block_size = seg->proc_count;
            blocklist_head->num_blocks = 1;
            blocklist_head->next = NULL;
            blocklist_tail = blocklist_head;
        }
        else if ((blocklist_tail->start_node_id + blocklist_tail->num_blocks == seg->node_id)
                 && (blocklist_tail->block_size == seg->proc_count)) {
            blocklist_tail->num_blocks++;
        }
        else {
            HYDU_MALLOC(blocklist_tail->next, struct block *, sizeof(struct block), status);
            blocklist_tail = blocklist_tail->next;
            blocklist_tail->start_node_id = seg->node_id;
            blocklist_tail->block_size = seg->proc_count;
            blocklist_tail->num_blocks = 1;
            blocklist_tail->next = NULL;
        }
    }

    if (type == HYD_pmcd_pmi_vector) {
        i = 0;
        tmp[i++] = HYDU_strdup("(");
        tmp[i++] = HYDU_strdup("vector,");
        for (block = blocklist_head; block; block = block->next) {
            tmp[i++] = HYDU_strdup("(");
            tmp[i++] = HYDU_int_to_str(block->start_node_id);
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
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized process mapping\n");
    }

    for (seg = seglist_head; seg; seg = nseg) {
        nseg = seg->next;
        HYDU_FREE(seg);
    }
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


static struct HYD_pmcd_pmi_node *find_node(HYD_pmcd_pmi_pg_t * pg, int rank)
{
    int node_id, srank;
    struct HYD_proxy *proxy;
    struct HYD_pmcd_pmi_node *node, *tmp;
    HYD_status status = HYD_SUCCESS;

    srank = rank % HYD_handle.global_core_count;

    node_id = 0;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        if ((srank >= proxy->start_pid) &&
            (srank < (proxy->start_pid + proxy->node.core_count)))
            break;
        node_id++;
    }

    /* See if the node already exists */
    for (node = pg->node_list; node; node = node->next)
        if (node->node_id == node_id)
            return node;

    /* allocate node-level KVS space */
    node = allocate_node(pg, node_id);
    HYDU_ERR_CHKANDJUMP(status, !node, HYD_INTERNAL_ERROR, "unable to allocate PMI node\n");

    status = allocate_kvs(&node->kvs, pg->pgid);
    HYDU_ERR_POP(status, "unable to allocate kvs space\n");

    if (pg->node_list == NULL)
        pg->node_list = node;
    else {
        tmp = pg->node_list;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = node;
    }

  fn_exit:
    return node;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_pmcd_pmi_add_process_to_pg(HYD_pmcd_pmi_pg_t * pg, int fd, int rank)
{
    HYD_pmcd_pmi_process_t *process, *tmp;
    struct HYD_pmcd_pmi_node *node;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the node corresponding to the rank */
    node = find_node(pg, rank);

    /* Add process to the node */
    HYDU_MALLOC(process, HYD_pmcd_pmi_process_t *, sizeof(HYD_pmcd_pmi_process_t), status);
    process->fd = fd;
    process->rank = rank;
    process->epoch = 0;
    process->node = node;
    process->next = NULL;
    if (node->process_list == NULL)
        node->process_list = process;
    else {
        tmp = node->process_list;
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


HYD_pmcd_pmi_process_t *HYD_pmcd_pmi_find_process(int fd)
{
    HYD_pmcd_pmi_pg_t *pg;
    HYD_pmcd_pmi_node_t *node;
    HYD_pmcd_pmi_process_t *process = NULL;

    for (pg = HYD_pg_list; pg; pg = pg->next) {
        for (node = pg->node_list; node; node = node->next) {
            for (process = node->process_list; process; process = process->next) {
                if (process->fd == fd)
                    return process;
            }
        }
    }

    return NULL;
}


HYD_status HYD_pmcd_pmi_init(void)
{
    struct HYD_proxy *proxy;
    struct HYD_proxy_exec *exec;
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_pmcd_create_pg(&HYD_pg_list, 0);
    HYDU_ERR_POP(status, "unable to create pg\n");

    /* Find the number of processes in the PG */
    HYD_pg_list->num_subgroups = 0;
    for (proxy = HYD_handle.pg_list.proxy_list; proxy; proxy = proxy->next) {
        for (exec = proxy->exec_list; exec; exec = exec->next)
            HYD_pg_list->num_subgroups += exec->proc_count;
    }

    if (HYD_handle.ranks_per_proc != -1)
        HYD_pg_list->num_procs = HYD_pg_list->num_subgroups * HYD_handle.ranks_per_proc;
    else
        HYD_pg_list->num_procs = HYD_pg_list->num_subgroups;

    /* Allocate and initialize the connected ranks */
    HYDU_MALLOC(HYD_pg_list->conn_procs, int *, HYD_pg_list->num_subgroups * sizeof(int),
                status);
    for (i = 0; i < HYD_pg_list->num_subgroups; i++)
        HYD_pg_list->conn_procs[i] = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_status HYD_pmcd_pmi_finalize(void)
{
    HYD_pmcd_pmi_pg_t *pg, *tmp;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg = HYD_pg_list;
    while (pg) {
        tmp = pg->next;

        if (pg->conn_procs)
            HYDU_FREE(pg->conn_procs);

        status = free_pmi_node_list(pg->node_list);
        HYDU_ERR_POP(status, "unable to free process list\n");

        status = free_pmi_kvs_list(pg->kvs);
        HYDU_ERR_POP(status, "unable to free kvs list\n");

        HYDU_FREE(pg);
        pg = tmp;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
