/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LSF_H_INCLUDED
#define LSF_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_lsf_query_native_int(int *ret);
HYD_status HYDT_bscd_lsf_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_lsf_query_env_inherit(const char *env_name, int *ret);

#endif /* LSF_H_INCLUDED */
