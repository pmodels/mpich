#ifndef MPIU_COLL_SELECTION_TREE_TYPES_H_INCLUDED
#define MPIU_COLL_SELECTION_TREE_TYPES_H_INCLUDED

#include "coll_selection_tree_pre.h"

typedef enum {
    MPIU_COLL_SELECTION_INTRA_COMM,
    MPIU_COLL_SELECTION_INTER_COMM,
    MPIU_COLL_SELECTION_COMM_KIND_NUM
} MPIU_COLL_SELECTION_comm_kind_t;

typedef enum {
    MPIU_COLL_SELECTION_FLAT_COMM,
    MPIU_COLL_SELECTION_TOPO_COMM,
    MPIU_COLL_SELECTION_COMM_HIERARCHY_NUM
} MPIU_COLL_SELECTION_comm_hierarchy_kind_t;

typedef enum {
    MPIU_COLL_SELECTION_STORAGE,
    MPIU_COLL_SELECTION_COMM_KIND,
    MPIU_COLL_SELECTION_COMM_HIERARCHY,
    MPIU_COLL_SELECTION_COLLECTIVE,
    MPIU_COLL_SELECTION_COMMSIZE,
    MPIU_COLL_SELECTION_MSGSIZE,
    MPIU_COLL_SELECTION_CONTAINER,
    MPIU_COLL_SELECTION_TYPES_NUM,
    MPIU_COLL_SELECTION_DEFAULT_TERMINAL_NODE_TYPE = MPIU_COLL_SELECTION_CONTAINER,
    MPIU_COLL_SELECTION_DEFAULT_NODE_TYPE = -1
} MPIU_COLL_SELECTION_node_type_t;

typedef struct MPIU_COLL_SELECTION_tree_node {
    MPIU_COLL_SELECTION_storage_handler parent;
    MPIU_COLL_SELECTION_node_type_t type;
    MPIU_COLL_SELECTION_node_type_t next_layer_type;
    int key;
    int children_count;
    int cur_child_idx;
    union {
        MPIU_COLL_SELECTION_storage_handler offset[0];
        MPIDIG_coll_algo_generic_container_t containers[0];
    };
} MPIU_COLL_SELECTION_tree_node_t;

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_node(MPIU_COLL_SELECTION_storage_handler parent,
                                MPIU_COLL_SELECTION_node_type_t node_type,
                                MPIU_COLL_SELECTION_node_type_t next_layer_type,
                                int node_key, int children_count);

MPIU_COLL_SELECTION_storage_handler
MPIU_COLL_SELECTION_create_leaf(MPIU_COLL_SELECTION_storage_handler parent,
                                int node_type, int containers_count, void *containers);

void *MPIU_COLL_SELECTION_get_container(MPIU_COLL_SELECTION_storage_handler node);

#endif /* MPIU_COLL_SELECTION_TREE_TYPES_H_INCLUDED */
