/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LL_H_INCLUDED
#define LL_H_INCLUDED

#include "hydra.h"

HYD_status HYDTI_bscd_ll_query_node_count(int *count);

HYD_status HYDT_bscd_ll_launch_procs(char **args, struct HYD_proxy *proxy_list, int use_rmk,
                                     int *control_fd);
HYD_status HYDT_bscd_ll_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_ll_query_native_int(int *ret);
HYD_status HYDT_bscd_ll_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_ll_query_env_inherit(const char *env_name, int *ret);

#endif /* LL_H_INCLUDED */
