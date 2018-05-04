#ifndef MPIU_COLL_SELECTION_TREE_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_H_INCLUDED

#include <sys/stat.h>
#include "mpiimpl.h"
#include "coll_tree_bin_types.h"

MPIU_COLL_SELECTION_storage_handler MPIU_COLL_SELECTION_tree_current_offset = 0;
char MPIU_COLL_SELECTION_offset_tree[MPIU_COLL_SELECTION_STORAGE_SIZE];

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_node(MPIU_COLL_SELECTION_storage_handler parent,
                                MPIU_COLL_SELECTION_node_type_t node_type,
                                MPIU_COLL_SELECTION_node_type_t next_layer_type,
                                int node_key, int children_count)
{
    int node_offset = 0;
    int child_index = 0;
    MPIU_COLL_SELECTION_storage_handler tmp = MPIU_COLL_SELECTION_tree_current_offset;

    node_offset =
        sizeof(MPIU_COLL_SELECTION_tree_node_t) +
        sizeof(MPIU_COLL_SELECTION_storage_handler) * children_count;
    if (parent != MPIU_COLL_SELECTION_NULL_ENTRY) {
        child_index = MPIU_COLL_SELECTION_NODE_FIELD(parent, cur_child_idx);
        MPIU_COLL_SELECTION_NODE_FIELD(parent, offset[child_index]) = tmp;
        MPIU_COLL_SELECTION_NODE_FIELD(parent, cur_child_idx)++;
    }

    MPIU_COLL_SELECTION_tree_current_offset += node_offset;
    MPIR_Assertp(MPIU_COLL_SELECTION_tree_current_offset <= MPIU_COLL_SELECTION_STORAGE_SIZE);

    MPIU_COLL_SELECTION_NODE_FIELD(tmp, parent) = parent;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, cur_child_idx) = 0;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, type) = node_type;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, next_layer_type) = next_layer_type;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, key) = node_key;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, children_count) = children_count;

    return tmp;
}

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_leaf(MPIU_COLL_SELECTION_storage_handler parent, int node_type,
                                int containers_count, void *containers)
{
    int node_offset = 0;
    MPIU_COLL_SELECTION_storage_handler tmp = MPIU_COLL_SELECTION_tree_current_offset;

    node_offset =
        sizeof(MPIU_COLL_SELECTION_tree_node_t) +
        sizeof(MPIDIG_coll_algo_generic_container_t) * containers_count;
    if (parent != MPIU_COLL_SELECTION_NULL_ENTRY)
        MPIU_COLL_SELECTION_NODE_FIELD(parent, offset[0]) = tmp;
    memset(MPIU_COLL_SELECTION_HANDLER_TO_POINTER(tmp), 0, node_offset);
    MPIU_COLL_SELECTION_tree_current_offset += node_offset;

    MPIU_COLL_SELECTION_NODE_FIELD(tmp, parent) = parent;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, type) = node_type;
    MPIU_COLL_SELECTION_NODE_FIELD(tmp, children_count) = containers_count;
    if (containers != NULL) {
        MPIR_Memcpy((void *) MPIU_COLL_SELECTION_NODE_FIELD(tmp, containers), containers,
                    sizeof(MPIDIG_coll_algo_generic_container_t) * containers_count);
    }

    return tmp;
}

void *MPIU_COLL_SELECTION_get_container(MPIU_COLL_SELECTION_storage_handler node)
{
    if ((MPIU_COLL_SELECTION_NODE_FIELD(node, type) == MPIU_COLL_SELECTION_CONTAINER) &&
        MPIU_COLL_SELECTION_NODE_FIELD(node, containers) != NULL) {
        return (void *) MPIU_COLL_SELECTION_NODE_FIELD(node, containers);
    } else {
        return NULL;
    }
}

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_get_node_parent(MPIU_COLL_SELECTION_storage_handler node)
{
    return MPIU_COLL_SELECTION_NODE_FIELD(node, parent);
}

#endif /* MPIU_COLL_SELECTION_TREE_H_INCLUDED */
