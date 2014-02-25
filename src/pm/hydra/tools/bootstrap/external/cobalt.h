/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef COBALT_H_INCLUDED
#define COBALT_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_cobalt_query_native_int(int *ret);
HYD_status HYDT_bscd_cobalt_query_node_list(struct HYD_node **node_list);

#endif /* COBALT_H_INCLUDED */
