/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SGE_H_INCLUDED
#define SGE_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_sge_query_native_int(int *ret);
HYD_status HYDT_bscd_sge_query_node_list(struct HYD_node **node_list);
HYD_status HYDT_bscd_sge_query_env_inherit(const char *env_name, int *ret);

#endif /* SGE_H_INCLUDED */
