/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PBS_H_INCLUDED
#define PBS_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_pbs_query_native_int(int *ret);
HYD_status HYDT_bscd_pbs_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_pbs_query_jobid(char **jobid);

#endif /* PBS_H_INCLUDED */
