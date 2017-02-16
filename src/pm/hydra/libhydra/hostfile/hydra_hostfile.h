/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef HYDRA_HOSTFILE_H_INCLUDED
#define HYDRA_HOSTFILE_H_INCLUDED

#include "hydra_base.h"
#include "hydra_node.h"

HYD_status HYD_hostfile_process_tokens(char **tokens, struct HYD_node *node);
HYD_status HYD_hostfile_parse(const char *hostfile, int *node_count, struct HYD_node **nodes,
                              HYD_status(*process_tokens) (char **tokens, struct HYD_node * node));

#endif /* HYDRA_HOSTFILE_H_INCLUDED */
