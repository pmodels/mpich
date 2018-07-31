/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_TREE_H_INCLUDED
#define HYDRA_TREE_H_INCLUDED

#define HYD_TREE_DEFAULT_WIDTH  (16)

int HYD_tree_start(int num_nodes, int idx, int tree_width);
int HYD_tree_size(int num_nodes, int idx, int tree_width);

#endif /* HYDRA_TREE_H_INCLUDED */
