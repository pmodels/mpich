/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef HYDRA_RMK_SGE_H_INCLUDED
#define HYDRA_RMK_SGE_H_INCLUDED

#include "hydra_base.h"
#include "hydra_node.h"

int HYDI_rmk_sge_detect(void);
HYD_status HYDI_rmk_sge_query_node_list(int *node_count, struct HYD_node **nodes);

#endif /* HYDRA_RMK_SGE_H_INCLUDED */
