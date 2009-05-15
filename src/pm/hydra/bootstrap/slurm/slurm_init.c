/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bsci.h"
#include "slurm.h"

struct HYD_BSCI_fns HYD_BSCI_fns;

HYD_Status HYD_BSCI_slurm_init(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_BSCI_fns.launch_procs = HYD_BSCD_slurm_launch_procs;
    HYD_BSCI_fns.query_partition_id = HYD_BSCD_slurm_query_partition_id;
    HYD_BSCI_fns.query_node_list = HYD_BSCD_slurm_query_node_list;

    HYDU_FUNC_EXIT();

    return status;
}
