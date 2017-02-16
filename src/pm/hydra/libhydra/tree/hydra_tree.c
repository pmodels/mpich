/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_tree.h"

int HYD_tree_start(int num_nodes, int idx, int tree_width)
{
    int ret = idx * (num_nodes / tree_width);

    if (idx <= num_nodes % tree_width)
        ret += idx;
    else
        ret += num_nodes % tree_width;

    return ret;
}

int HYD_tree_size(int num_nodes, int idx, int tree_width)
{
    return (num_nodes / tree_width) + (idx < num_nodes % tree_width);
}
