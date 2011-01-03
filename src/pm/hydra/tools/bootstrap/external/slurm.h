/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SLURM_H_INCLUDED
#define SLURM_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_slurm_launch_procs(char **args, struct HYD_node *node_list,
                                        int *control_fd, int enable_stdin);
HYD_status HYDT_bscd_slurm_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_slurm_query_node_list(struct HYD_node **node_list);

#endif /* SLURM_H_INCLUDED */
