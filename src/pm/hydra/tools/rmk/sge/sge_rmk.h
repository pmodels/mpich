/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SGE_RMK_H_INCLUDED
#define SGE_RMK_H_INCLUDED

#include "hydra.h"

int HYDT_rmki_sge_detect(void);
HYD_status HYDT_rmki_sge_query_node_list(struct HYD_node **node_list);

#endif /* SGE_RMK_H_INCLUDED */
