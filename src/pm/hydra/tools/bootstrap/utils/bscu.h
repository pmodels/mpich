/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BSCU_H_INCLUDED
#define BSCU_H_INCLUDED

#include "hydra_base.h"

HYD_status HYDT_bscu_finalize(void);
HYD_status HYDT_bscu_wait_for_completion(struct HYD_proxy *proxy_list);
HYD_status HYDT_bscu_query_node_list(int *num_nodes, struct HYD_proxy **proxy_list);
HYD_status HYDT_bscu_query_usize(int *size);
HYD_status HYDT_bscu_query_proxy_id(int *proxy_id);

#endif /* BSCU_H_INCLUDED */
