/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef RMKU_H_INCLUDED
#define RMKU_H_INCLUDED

#include "hydra_base.h"

HYD_status HYD_rmku_query_node_list(struct HYD_node **node_list);
HYD_status HYD_rmku_query_native_int(int *ret);

#endif /* RMKU_H_INCLUDED */
