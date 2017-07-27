/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_NODE_H_INCLUDED
#define HYDRA_NODE_H_INCLUDED

#include "hydra_base.h"

#define HYD_MAX_USERNAME_LEN   (32)

struct HYD_node {
    char username[HYD_MAX_USERNAME_LEN];
    char hostname[HYD_MAX_HOSTNAME_LEN];
    int core_count;
    int node_id;
};

HYD_status HYD_node_list_append(const char *hostname, int num_procs, struct HYD_node **nodes,
                                int *node_count, int *max_node_count);

#endif /* HYDRA_NODE_H_INCLUDED */
