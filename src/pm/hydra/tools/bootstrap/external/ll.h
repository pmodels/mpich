/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LL_H_INCLUDED
#define LL_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_ll_launch_procs(char **args, struct HYD_node *node_list,
                                     int *control_fd, int enable_stdin);
HYD_status HYDT_bscd_ll_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_ll_query_node_list(struct HYD_node **node_list);
HYD_status HYDTI_bscd_ll_query_node_count(int *count);

#endif /* LL_H_INCLUDED */
