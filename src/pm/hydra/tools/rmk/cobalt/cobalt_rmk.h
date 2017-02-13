/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef COBALT_RMK_H_INCLUDED
#define COBALT_RMK_H_INCLUDED

#include "hydra.h"

int HYDT_rmki_cobalt_detect(void);
HYD_status HYDT_rmki_cobalt_query_node_list(struct HYD_node **node_list);

#endif /* COBALT_RMK_H_INCLUDED */
