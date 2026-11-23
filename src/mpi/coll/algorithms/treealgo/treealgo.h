/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TREEALGO_H_INCLUDED
#define TREEALGO_H_INCLUDED

#include "treealgo_types.h"

int MPII_Treealgo_init(void);
int MPII_Treealgo_comm_init(MPIR_Comm * comm);
int MPII_Treealgo_comm_cleanup(MPIR_Comm * comm);
int MPIR_Treealgo_tree_create(int rank, int nranks, int tree_type, int k, int root,
                              MPIR_Treealgo_tree_t * ct);
int MPIR_Treealgo_tree_create_topo_aware(MPIR_Comm * comm, int tree_type, int k, int root,
                                         bool enable_reorder, MPIR_Treealgo_tree_t * ct);
int MPIR_Treealgo_tree_create_topo_wave(MPIR_Comm * comm, int k, int root,
                                        bool enable_reorder, int overhead, int lat_diff_groups,
                                        int lat_diff_switches, int lat_same_switches,
                                        MPIR_Treealgo_tree_t * ct);
void MPIR_Treealgo_tree_free(MPIR_Treealgo_tree_t * tree);

/* For tree builders: an empty tree (no parent, no children), adding a child, and a copy that
 * owns its own children. */
void MPIR_Treealgo_tree_init(MPIR_Treealgo_tree_t * t, int rank, int nranks);
int MPII_Treealgo_tree_add_child_heap(MPIR_Treealgo_tree_t * t, int child);
int MPIR_Treealgo_tree_copy(MPIR_Treealgo_tree_t * dst, const MPIR_Treealgo_tree_t * src);

static inline int MPIR_Treealgo_tree_add_child(MPIR_Treealgo_tree_t * t, int child)
{
    if (!t->heap_children && t->num_children < MPIR_TREEALGO_INLINE_CHILDREN) {
        t->inl_children[t->num_children++] = child;
        return MPI_SUCCESS;
    }
    return MPII_Treealgo_tree_add_child_heap(t, child);
}

#endif /* TREEALGO_H_INCLUDED */
