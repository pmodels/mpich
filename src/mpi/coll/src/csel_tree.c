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
