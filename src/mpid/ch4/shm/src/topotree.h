/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef TOPOTREE_H_INCLUDED
#define TOPOTREE_H_INCLUDED

#include "topotree_types.h"

int MPIDI_SHM_create_template_tree(MPIDI_SHM_topotree_t * template_tree, int k_val,
                                   bool right_skewed, int max_ranks, MPIR_Errflag_t * eflag);

void MPIDI_SHM_hwloc_init_bindmap(int num_ranks, int topo_depth, int *shared_region,
                                  int **bind_map);

void MPIDI_SHM_copy_tree(int *shared_region, int num_ranks, int rank,
                         MPIR_Treealgo_tree_t * my_tree, int *topotree_fail);

int MPIDI_SHM_topotree_get_package_level(int topo_depth, int *max_entries_per_level, int num_ranks,
                                         int **bind_map);

void MPIDI_SHM_gen_package_tree(int num_packages, int k_val, MPIDI_SHM_topotree_t * package_tree,
                                int *package_leaders);

void MPIDI_SHM_gen_tree_sharedmemory(int *shared_region, MPIDI_SHM_topotree_t * tree,
                                     MPIDI_SHM_topotree_t * package_tree, int *package_leaders,
                                     int num_packages, int num_ranks, int k_val,
                                     bool package_leaders_first);

int MPIDI_SHM_gen_tree(int k_val, int *shared_region, int *max_entries_per_level,
                       int **ranks_per_package, int max_ranks_per_package, int *package_ctr,
                       int package_level, int num_ranks, bool package_leaders_first,
                       bool right_skewed, MPIR_Errflag_t * eflag);

int MPIDI_SHM_topology_tree_init(MPIR_Comm * comm_ptr, int root, int bcast_k,
                                 MPIR_Treealgo_tree_t * bcast_tree, int *bcast_topotree_fail,
                                 int reduce_k, MPIR_Treealgo_tree_t * reduce_tree,
                                 int *reduce_topotree_fail, MPIR_Errflag_t * eflag);

#endif /* TOPOTREE_H_INCLUDED */
