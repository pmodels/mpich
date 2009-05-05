/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "pmi_handle.h"
#include "pmi_handle_v1.h"
#include "pmi_handle_v2.h"

HYD_Handle handle;
HYD_PMCD_pmi_pg_t *pg_list = NULL;

struct HYD_PMCD_pmi_handle *HYD_PMCD_pmi_v1;
struct HYD_PMCD_pmi_handle *HYD_PMCD_pmi_v2;

static HYD_Status allocate_kvs(HYD_PMCD_pmi_kvs_t ** kvs, int pgid)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*kvs, HYD_PMCD_pmi_kvs_t *, sizeof(HYD_PMCD_pmi_kvs_t), status);
    HYDU_snprintf((*kvs)->kvs_name, MAXNAMELEN, "kvs_%d_%d", (int) getpid(), pgid);
    (*kvs)->key_pair = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static HYD_Status create_pg(HYD_PMCD_pmi_pg_t ** pg, int pgid)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*pg, HYD_PMCD_pmi_pg_t *, sizeof(HYD_PMCD_pmi_pg_t), status);
    (*pg)->id = pgid;
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


static HYD_Status free_pmi_process_list(HYD_PMCD_pmi_process_t * process_list)
{
    HYD_PMCD_pmi_process_t *process, *tmp;
    HYD_Status status = HYD_SUCCESS;

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


static HYD_Status free_pmi_kvs_list(HYD_PMCD_pmi_kvs_t * kvs_list)
{
    HYD_PMCD_pmi_kvs_pair_t *key_pair, *tmp;
    HYD_Status status = HYD_SUCCESS;

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


static HYD_Status free_pmi_node_list(HYD_PMCD_pmi_node_t * node_list)
{
    HYD_PMCD_pmi_node_t *node, *tmp;
    HYD_Status status = HYD_SUCCESS;

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


static struct HYD_PMCD_pmi_node *allocate_node(HYD_PMCD_pmi_pg_t * pg, int node_id)
{
    struct HYD_PMCD_pmi_node *node;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(node, HYD_PMCD_pmi_node_t *, sizeof(HYD_PMCD_pmi_node_t), status);
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


HYD_Status HYD_PMCD_pmi_add_kvs(char *key, char *val, HYD_PMCD_pmi_kvs_t * kvs,
                                char **key_pair_str, int *ret)
{
    HYD_PMCD_pmi_kvs_pair_t *key_pair, *run;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(key_pair, HYD_PMCD_pmi_kvs_pair_t *, sizeof(HYD_PMCD_pmi_kvs_pair_t), status);
    HYDU_snprintf(key_pair->key, MAXKEYLEN, "%s", key);
    HYDU_snprintf(key_pair->val, MAXVALLEN, "%s", val);
    key_pair->next = NULL;

    key_pair_str = NULL;
    *ret = 0;

    if (kvs->key_pair == NULL) {
        kvs->key_pair = key_pair;
    }
    else {
        run = kvs->key_pair;
        while (run->next) {
            if (!strcmp(run->key, key_pair->key)) {
                /* duplicate key found */
                *key_pair_str = HYDU_strdup(key_pair->key);
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


HYD_Status HYD_PMCD_pmi_id_to_rank(int id, int *rank)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (handle.ranks_per_proc == -1) {
        /* If multiple procs per rank is not defined, use ID as the rank */
        *rank = id;
    }
    else {
        *rank = (id * handle.ranks_per_proc) + pg_list->conn_procs[id];
        pg_list->conn_procs[id]++;
    }

    HYDU_FUNC_EXIT();
    return status;
}


HYD_Status HYD_PMCD_pmi_process_mapping(HYD_PMCD_pmi_process_t *process,
                                        enum HYD_PMCD_pmi_process_mapping_type type,
                                        char **process_mapping_str)
{
    int i, j, rank, node_id, process_id, rem, *process_mapping;
    char *tmp[HYD_NUM_TMP_STRINGS];
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    struct HYD_Partition_segment *segment;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    if (type == HYD_PMCD_pmi_explicit) {
        /* Explicit process mapping */
        HYDU_MALLOC(process_mapping, int *, process->node->pg->num_procs * sizeof(int),
                    status);

        /* We search all executables with our PGID in each partition
         * of handle. */
        rank = -1;
        node_id = -1;
        for (partition = handle.partition_list; partition; partition = partition->next) {
            node_id++;
            process_id = 0;
            for (exec = partition->exec_list; exec; exec = exec->next) {
                if (exec->pgid == process->node->pg->id) {
                    for (i = 0; i < exec->proc_count; i++) {
                        /* Figure out what this process' rank will be */
                        rank = ((process_id / partition->one_pass_count) *
                                handle.one_pass_count);
                        rem = process_id - rank;

                        for (segment = partition->segment_list; segment;
                             segment = segment->next) {
                            if (rem >= segment->proc_count)
                                rem -= segment->proc_count;
                            else {
                                rank += segment->start_pid + rem;
                                break;
                            }
                        }

                        process_mapping[rank] = node_id;
                        process_id++;
                    }
                }
                else
                    break;
            }
        }

        i = 0;
        tmp[i++] = HYDU_strdup("explicit,");
        for (j = 0; j < process->node->pg->num_procs; j++) {
            tmp[i++] = HYDU_int_to_str(process_mapping[j]);
            if (j < process->node->pg->num_procs - 1)
                tmp[i++] = HYDU_strdup(",");
        }
        tmp[i++] = NULL;

        status = HYDU_str_alloc_and_join(tmp, process_mapping_str);
        HYDU_ERR_POP(status, "error while joining strings\n");

        for (i = 0; tmp[i]; i++)
            HYDU_FREE(tmp[i]);
    }
    else if (type == HYD_PMCD_pmi_vector) {
    }
    else {
        HYDU_ERR_SETANDJUMP(status, HYD_INTERNAL_ERROR, "unrecognized process mapping\n");
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


static struct HYD_PMCD_pmi_node *find_node(HYD_PMCD_pmi_pg_t * pg, int rank)
{
    int found = 0, node_id, srank;
    struct HYD_Partition *partition;
    struct HYD_Partition_segment *segment;
    struct HYD_PMCD_pmi_node *node, *tmp;
    HYD_Status status = HYD_SUCCESS;

    srank = rank % handle.one_pass_count;

    node_id = 0;
    for (partition = handle.partition_list; partition; partition = partition->next) {
        for (segment = partition->segment_list; segment; segment = segment->next) {
            if ((srank >= segment->start_pid) &&
                (srank < (segment->start_pid + segment->proc_count))) {
                /* We found our rank */
                found = 1;
                break;
            }
        }
        if (found)
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

    status = allocate_kvs(&node->kvs, 0);
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

HYD_Status HYD_PMCD_pmi_add_process_to_pg(HYD_PMCD_pmi_pg_t * pg, int fd, int rank)
{
    HYD_PMCD_pmi_process_t *process, *tmp;
    struct HYD_PMCD_pmi_node *node;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    /* Find the node corresponding to the rank */
    node = find_node(pg, rank);

    /* Add process to the node */
    HYDU_MALLOC(process, HYD_PMCD_pmi_process_t *, sizeof(HYD_PMCD_pmi_process_t), status);
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


HYD_PMCD_pmi_process_t *HYD_PMCD_pmi_find_process(int fd)
{
    HYD_PMCD_pmi_pg_t *pg;
    HYD_PMCD_pmi_node_t *node;
    HYD_PMCD_pmi_process_t *process = NULL;

    for (pg = pg_list; pg; pg = pg->next) {
        for (node = pg->node_list; node; node = node->next) {
            for (process = node->process_list; process; process = process->next) {
                if (process->fd == fd)
                    return process;
            }
        }
    }

    return NULL;
}


HYD_Status HYD_PMCD_pmi_init(void)
{
    struct HYD_Partition *partition;
    struct HYD_Partition_exec *exec;
    int i;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = create_pg(&pg_list, 0);
    HYDU_ERR_POP(status, "unable to create pg\n");

    /* Find the number of processes in the PG */
    pg_list->num_subgroups = 0;
    for (partition = handle.partition_list; partition && partition->exec_list;
         partition = partition->next)
        for (exec = partition->exec_list; exec; exec = exec->next)
            pg_list->num_subgroups += exec->proc_count;

    if (handle.ranks_per_proc != -1)
        pg_list->num_procs = pg_list->num_subgroups * handle.ranks_per_proc;
    else
        pg_list->num_procs = pg_list->num_subgroups;

    /* Allocate and initialize the connected ranks */
    HYDU_MALLOC(pg_list->conn_procs, int *, pg_list->num_subgroups * sizeof(int), status);
    for (i = 0; i < pg_list->num_subgroups; i++)
        pg_list->conn_procs[i] = 0;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}


HYD_Status HYD_PMCD_pmi_finalize(void)
{
    HYD_PMCD_pmi_pg_t *pg, *tmp;
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    pg = pg_list;
    while (pg) {
        tmp = pg->next;

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
