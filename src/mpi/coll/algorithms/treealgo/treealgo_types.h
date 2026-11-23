/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TREEALGO_TYPES_H_INCLUDED
#define TREEALGO_TYPES_H_INCLUDED

/* A rank's view of a tree: its parent and children. The children are stored in the tree itself
 * while they fit, so building a common tree allocates nothing; more children spill to the heap.
 * Read them with MPIR_Treealgo_tree_children(). */
#define MPIR_TREEALGO_INLINE_CHILDREN 16

typedef struct {
    int rank;
    int nranks;
    int parent;
    int num_children;
    int max_children;           /* capacity of heap_children; 0 while the children fit inline */
    int *heap_children;
    int inl_children[MPIR_TREEALGO_INLINE_CHILDREN];
} MPIR_Treealgo_tree_t;

static inline int *MPIR_Treealgo_tree_children(MPIR_Treealgo_tree_t * t)
{
    return t->heap_children ? t->heap_children : t->inl_children;
}

#endif /* TREEALGO_TYPES_H_INCLUDED */
