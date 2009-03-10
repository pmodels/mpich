/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_mem.h"
#include "hydra_launch.h"

HYD_Status HYDU_Allocate_Partition(struct HYD_Partition_list **partition)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDU_MALLOC(*partition, struct HYD_Partition_list *,
                sizeof(struct HYD_Partition_list), status);
    (*partition)->name = NULL;
    (*partition)->proc_count = 0;
    (*partition)->mapping = NULL;
    (*partition)->group_id = -1;
    (*partition)->group_rank = -1;
    (*partition)->pid = -1;
    (*partition)->out = -1;
    (*partition)->err = -1;
    (*partition)->exit_status = -1;
    (*partition)->args[0] = NULL;

    (*partition)->next = NULL;

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
