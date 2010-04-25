/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef POE_H_INCLUDED
#define POE_H_INCLUDED

#include "hydra_base.h"

HYD_status HYDT_bscd_poe_launch_procs(char **args, struct HYD_node *node_list,
                                      int *control_fd, int enable_stdin,
                                      HYD_status(*stdout_cb) (void *buf, int buflen),
                                      HYD_status(*stderr_cb) (void *buf, int buflen));
HYD_status HYDT_bscd_poe_query_proxy_id(int *proxy_id);
HYD_status HYDT_bscd_poe_query_node_list(struct HYD_node **node_list);

extern int HYDT_bscd_poe_user_node_list;

#endif /* POE_H_INCLUDED */
