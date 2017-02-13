/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PBS_RMK_H_INCLUDED
#define PBS_RMK_H_INCLUDED

#include "hydra.h"

int HYDT_rmki_pbs_detect(void);
HYD_status HYDT_rmki_pbs_query_node_list(struct HYD_node **node_list);

#endif /* PBS_RMK_H_INCLUDED */
