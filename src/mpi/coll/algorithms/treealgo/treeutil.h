/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TREEUTIL_H_INCLUDED
#define TREEUTIL_H_INCLUDED

/* Generate kary tree information for rank 'rank' */
int MPII_Treeutil_tree_kary_init(int rank, int nranks, int k, int root, MPIR_Treealgo_tree_t * ct);

/* Generate knomial_1 tree information for rank 'rank' */
int MPII_Treeutil_tree_knomial_1_init(int rank, int nranks, int k, int root,
                                      MPIR_Treealgo_tree_t * ct);

/* Generate knomial_2 tree information for rank 'rank' */
int MPII_Treeutil_tree_knomial_2_init(int rank, int nranks, int k, int root,
                                      MPIR_Treealgo_tree_t * ct);

#endif /* TREEUTIL_H_INCLUDED */
