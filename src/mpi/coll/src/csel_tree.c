/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "csel_internal.h"

csel_node_s *csel_node_create(csel_node_type_e type)
{
    csel_node_s *new_node = (csel_node_s *) MPL_malloc(sizeof(csel_node_s), MPL_MEM_OTHER);

    MPIR_Assert(new_node);

    new_node->type = type;
    new_node->success = NULL;
    new_node->failure = NULL;

    /* set further allocation and init */
    switch (type) {
        case CSEL_NODE_TYPE__CONTAINER:{
                MPII_Csel_container_s *new_container = (MPII_Csel_container_s *)
                    MPL_malloc(sizeof(MPII_Csel_container_s), MPL_MEM_OTHER);
                MPIR_Assert(new_container);
                new_node->u.cnt.container = new_container;
                break;
            }
        default:
            break;
    }

    return new_node;
}

void csel_node_free(csel_node_s ** node)
{
    if (*node == NULL) {
        return;
    }

    MPIR_Assert((*node)->success == NULL);
    MPIR_Assert((*node)->failure == NULL);

    switch ((*node)->type) {
        case CSEL_NODE_TYPE__CONTAINER:
            MPL_free((*node)->u.cnt.container);
            break;
        case CSEL_NODE_TYPE__OPERATOR__RANGE_SET:
            for (int idx = 0; idx < (*node)->u.range_set.size; idx++) {
                csel_tree_free(&(*node)->u.range_set.nodes[idx]);
            }
            MPL_free((*node)->u.range_set.nodes);
            break;
        default:
            break;
    }
    MPL_free(*node);
    *node = NULL;
}

void csel_tree_free(csel_node_s ** node)
{
    if (*node == NULL) {
        return;
    }

    csel_tree_free(&(*node)->success);
    csel_tree_free(&(*node)->failure);

    csel_node_free(node);
}

csel_node_s *csel_node_swap_success(csel_node_s * node, csel_node_s * new_succ)
{
    MPIR_Assert(node);
    csel_node_s *old_succ = node->success;
    node->success = new_succ;
    return old_succ;
}

csel_node_s *csel_node_swap_failure(csel_node_s * node, csel_node_s * new_fail)
{
    MPIR_Assert(node);
    csel_node_s *old_fail = node->failure;
    node->failure = new_fail;
    return old_fail;
}

/* update node content without touching child nodes */
void csel_node_update(csel_node_s * node, csel_node_s node_value)
{
    MPIR_Assert(node);
    node->type = node_value.type;
    node->u = node_value.u;
}

static void csel_tree_merge_anys(csel_node_s ** node);
static void csel_tree_convert_lt2le(csel_node_s ** node);
static void csel_tree_merge_le2range(csel_node_s ** node, csel_node_type_e le_type,
                                     csel_node_type_e range_type);
static void csel_tree_merge_comm_size_range(csel_node_s ** node);
static void csel_tree_merge_avg_msg_size_range(csel_node_s ** node);
static void csel_tree_merge_total_msg_size_range(csel_node_s ** node);
static void csel_tree_merge_count_range(csel_node_s ** node);
static void csel_tree_merge_comm_avg_ppn_range(csel_node_s ** node);

static void csel_tree_apply(csel_node_s ** node, void (*func) (csel_node_s ** node));

static void csel_tree_merge_anys(csel_node_s ** node)
{
    while ((*node)->type == CSEL_NODE_TYPE__OPERATOR__ANY) {
        csel_node_s *any_node = *node;
        *node = any_node->success;
        any_node->success = NULL;
        csel_node_free(&any_node);
    }
}

static void csel_tree_convert_lt2le(csel_node_s ** node)
{
    int val = 0;
    MPIR_Assert((*node));
    switch ((*node)->type) {
        case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT:
            val = (*node)->u.value_lt.val;
            MPIR_Assert(val > 0);
            (*node)->type = CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE;
            (*node)->u.value_le.val = val - 1;
            break;
        case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT:
            val = (*node)->u.value_lt.val;
            MPIR_Assert(val > 0);
            (*node)->type = CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE;
            (*node)->u.value_le.val = val - 1;
            break;
        case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT:
            val = (*node)->u.value_lt.val;
            MPIR_Assert(val > 0);
            (*node)->type = CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE;
            (*node)->u.value_le.val = val - 1;
            break;
        case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT:
            val = (*node)->u.value_lt.val;
            MPIR_Assert(val > 0);
            (*node)->type = CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE;
            (*node)->u.value_le.val = val - 1;
            break;
        default:
            break;
    }
}

static void csel_tree_merge_le2range(csel_node_s ** node, csel_node_type_e le_type,
                                     csel_node_type_e range_type)
{
    if (*node == NULL) {
        return;
    }

    int node_count = 0;
    int last_val = 0;
    for (csel_node_s * iter = (*node); iter != NULL && iter->type == le_type; iter = iter->failure) {
        node_count++;
        last_val = iter->u.value_le.val;
    }

    if (node_count == 0) {
        return;
    }

    if (node_count == 1 && last_val == INT_MAX) {
        /* there is only one range from 0 to INT_MAX, prune this node */
        csel_node_s *tmp = *node;
        *node = tmp->success;
        tmp->success = NULL;
        tmp->failure = NULL;
        csel_node_free(&tmp);
        return;
    }

    csel_node_s *range_node = csel_node_create(CSEL_NODE_TYPE__OPERATOR__RANGE_SET);
    if (last_val < INT_MAX) {
        range_node->u.range_set.size = node_count + 1;
    } else {
        range_node->u.range_set.size = node_count;
    }
    range_node->u.range_set.nodes = MPL_malloc(sizeof(csel_node_s *) * range_node->u.range_set.size,
                                               MPL_MEM_OTHER);
    csel_node_s *curr = (*node);
    int idx = 0;
    int prev_val = 0;
    for (; curr != NULL && curr->type == le_type; curr = curr->failure, idx++) {
        range_node->u.range_set.nodes[idx] = csel_node_create(range_type);
        range_node->u.range_set.nodes[idx]->u.range.val = curr->u.value_le.val;
        range_node->u.range_set.nodes[idx]->u.range.prev_val = prev_val;
        range_node->u.range_set.nodes[idx]->success = curr->success;
        prev_val = curr->u.value_le.val;
    }
    MPIR_Assert(idx == node_count);
    if (last_val < INT_MAX) {
        /* extra node for value higher than UB, which points to failure path of UB node */
        range_node->u.range_set.nodes[node_count] = csel_node_create(range_type);
        range_node->u.range_set.nodes[node_count]->u.range.val = INT_MAX;
        range_node->u.range_set.nodes[node_count]->u.range.prev_val = prev_val;
        range_node->u.range_set.nodes[node_count]->success = curr;
    }

    csel_node_s *old_node = (*node);
    *node = range_node;

    for (csel_node_s * tmp = NULL; old_node != NULL && old_node->type == le_type;) {
        tmp = old_node;
        old_node = old_node->failure;
        tmp->success = NULL;
        tmp->failure = NULL;
        csel_node_free(&tmp);
    }
}

static void csel_tree_merge_comm_size_range(csel_node_s ** node)
{
    csel_tree_merge_le2range(node, CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE,
                             CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_RANGE);
}

static void csel_tree_merge_avg_msg_size_range(csel_node_s ** node)
{
    csel_tree_merge_le2range(node, CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE,
                             CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_RANGE);
}

static void csel_tree_merge_total_msg_size_range(csel_node_s ** node)
{
    csel_tree_merge_le2range(node, CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE,
                             CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_RANGE);
}

static void csel_tree_merge_count_range(csel_node_s ** node)
{
    csel_tree_merge_le2range(node, CSEL_NODE_TYPE__OPERATOR__COUNT_LE,
                             CSEL_NODE_TYPE__OPERATOR__COUNT_RANGE);
}

static void csel_tree_merge_comm_avg_ppn_range(csel_node_s ** node)
{
    csel_tree_merge_le2range(node, CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE,
                             CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_RANGE);
}

static void csel_tree_apply(csel_node_s ** node, void (*func) (csel_node_s ** node))
{
    if (*node == NULL) {
        return;
    }

    switch ((*node)->type) {
        case CSEL_NODE_TYPE__OPERATOR__RANGE_SET:
            for (int idx = 0; idx < (*node)->u.range_set.size; idx++) {
                csel_tree_apply(&(*node)->u.range_set.nodes[idx], func);
            }
            break;
        default:
            func(node);
    }

    if ((*node)->success) {
        csel_tree_apply(&((*node)->success), func);
    }
    if ((*node)->failure) {
        csel_tree_apply(&((*node)->failure), func);
    }
}

void csel_tree_optimize(csel_node_s ** node)
{
    csel_tree_apply(node, csel_tree_merge_anys);
    csel_tree_apply(node, csel_tree_convert_lt2le);
    csel_tree_apply(node, csel_tree_merge_comm_size_range);
    csel_tree_apply(node, csel_tree_merge_avg_msg_size_range);
    csel_tree_apply(node, csel_tree_merge_total_msg_size_range);
    csel_tree_apply(node, csel_tree_merge_count_range);
    csel_tree_apply(node, csel_tree_merge_comm_avg_ppn_range);
}

csel_node_s *csel_tree_range_match(csel_node_s * node, int val)
{
    MPIR_Assert(node->type == CSEL_NODE_TYPE__OPERATOR__RANGE_SET);

    csel_node_s **array = node->u.range_set.nodes;
    int left = 0, right = node->u.range_set.size - 1;

    int mid = (left + right) / 2;

    while (true) {
        if (left == mid) {
            if (val <= array[mid]->u.range.val) {
                return array[mid]->success;
            } else {
                return array[mid + 1]->success;
            }
        }

        if (val > array[mid]->u.range.val) {
            left = mid;
            mid = (left + right) / 2;
        } else if (val < array[mid]->u.range.val) {
            right = mid;
            mid = (left + right) / 2;
        } else {
            return array[mid]->success;
        }
    }
}
