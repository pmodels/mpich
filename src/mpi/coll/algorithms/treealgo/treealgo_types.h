/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TREEALGO_TYPES_H_INCLUDED
#define TREEALGO_TYPES_H_INCLUDED

#include <utarray.h>

/* enumerator for different tree types */
typedef enum MPIR_Tree_type_t {
    MPIR_TREE_TYPE_KARY = 0,
    MPIR_TREE_TYPE_KNOMIAL_1,
    MPIR_TREE_TYPE_KNOMIAL_2,
    MPIR_TREE_TYPE_TOPOLOGY_AWARE,
    MPIR_TREE_TYPE_TOPOLOGY_AWARE_K,
    MPIR_TREE_TYPE_TOPOLOGY_WAVE,
} MPIR_Tree_type_t;

typedef struct {
    int rank;
    int nranks;
    int parent;
    int num_children;
    UT_array *children;
} MPIR_Treealgo_tree_t;

typedef struct {
    MPIR_Tree_type_t type;
    int root;
    union {
        struct {
            int k;
        } topo_aware;
        struct {
            int overhead;
            int lat_diff_groups;
            int lat_diff_switches;
            int lat_same_switches;
        } topo_wave;
    } u;
} MPIR_Treealgo_param_t;

#endif /* TREEALGO_TYPES_H_INCLUDED */
