/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TOPOTREE_H_INCLUDED
#define TOPOTREE_H_INCLUDED

#include "topotree_types.h"

int MPIDI_SHM_topology_tree_init(MPIR_Comm * comm_ptr, int root, int bcast_k,
                                 MPIR_Treealgo_tree_t * bcast_tree, int *bcast_topotree_fail,
                                 int reduce_k, MPIR_Treealgo_tree_t * reduce_tree,
                                 int *reduce_topotree_fail, MPIR_Errflag_t * eflag);

#endif /* TOPOTREE_H_INCLUDED */
