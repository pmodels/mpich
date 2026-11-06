/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* The search that selects the algorithm of a collective call, on every call, in the trees that
 * csel_load.c loads. */

#include "mpiimpl.h"
#include "coll_csel.h"

/* where MPIR_Csel_search starts: the dispatch, leaving nothing out */
static MPIR_Csel_node_s csel_dispatch_node = {.type = CSEL_NODE_TYPE__OPERATOR__DISPATCH };

MPII_Csel_container_s *MPIR_Csel_search(MPIR_Csel_coll_sig_s * coll_sig)
{
    /* A search follows the same path whenever it enters a subtree, so one that
     * makes more call= jumps than there are subtrees loops forever. */
    unsigned ncalls = 0;
    struct csel_dispatch_state state = { false, 0, 0 };

    MPIR_Csel_node_s *node = &csel_dispatch_node;
    while (node) {
        switch (node->type) {
            case CSEL_NODE_TYPE__OPERATOR__CALL:
                if (++ncalls > MPII_Csel_num_trees()) {
                    return NULL;
                }
                node = MPII_Csel_get_tree_by_idx(node->u.call.tree_idx);
                break;

            case CSEL_NODE_TYPE__OPERATOR__DISPATCH:{
                    if (++ncalls > MPII_Csel_num_trees()) {
                        return NULL;
                    }
                    int tree_idx = MPII_Csel_dispatch(coll_sig, node->u.dispatch.drop_mask, &state);
                    if (tree_idx < 0) {
                        return NULL;
                    }
                    node = MPII_Csel_get_tree_by_idx(tree_idx);
                    break;
                }

            case CSEL_NODE_TYPE__OPERATOR__COLLECTIVE:
                if (node->u.collective.coll_type == coll_sig->coll_type)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__ANY:
                node = node->success;
                break;

            case CSEL_NODE_TYPE__CONTAINER:
                return node->u.container;

            case CSEL_NODE_TYPE__ERROR:
                return NULL;

            default:
                node = MPII_Csel_run_condition(node, coll_sig);
                MPIR_Assert(node);
                break;
        }
    }

    /* unreachable, since every object ends with "any" */
    MPIR_Assert(0 && "MPIR_Csel_search failed to find an algorithm");
    return NULL;
}
