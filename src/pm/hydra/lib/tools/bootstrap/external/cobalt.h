/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COBALT_H_INCLUDED
#define COBALT_H_INCLUDED

#include "hydra.h"

HYD_status HYDT_bscd_cobalt_query_native_int(int *ret);
HYD_status HYDT_bscd_cobalt_query_node_list(struct HYD_node **node_list);

#endif /* COBALT_H_INCLUDED */
