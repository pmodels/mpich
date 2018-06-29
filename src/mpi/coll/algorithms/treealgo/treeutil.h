/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#ifndef TREEUTIL_H_INCLUDED
#define TREEUTIL_H_INCLUDED

/* Generate kary tree information for rank 'rank' */
int MPII_Treeutil_tree_kary_init(int rank, int nranks, int k, int root, MPII_Treealgo_tree_t * ct);

/* Generate knomial_1 tree information for rank 'rank' */
int MPII_Treeutil_tree_knomial_1_init(int rank, int nranks, int k, int root,
                                      MPII_Treealgo_tree_t * ct);

/* Generate knomial_2 tree information for rank 'rank' */
int MPII_Treeutil_tree_knomial_2_init(int rank, int nranks, int k, int root,
                                      MPII_Treealgo_tree_t * ct);

#endif /* TREEUTIL_H_INCLUDED */
