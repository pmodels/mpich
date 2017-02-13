/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LL_H_INCLUDED
#define LL_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_ll_launch_procs(const char *rmk, struct HYD_node *node_list, char **args,
                                     int *control_fd);
HYD_status HYDT_bscd_ll_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_ll_query_env_inherit(const char *env_name, int *ret);

#endif /* LL_H_INCLUDED */
