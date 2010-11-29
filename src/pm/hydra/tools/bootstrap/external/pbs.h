/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PBS_H_INCLUDED
#define PBS_H_INCLUDED

#include "hydra_base.h"

HYD_status HYDT_bscd_pbs_query_node_list(struct HYD_node **node_list);

extern int HYDT_bscd_pbs_user_node_list;

#endif /* PBS_H_INCLUDED */
