/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"

HYD_Status HYDU_alloc_partition(struct HYD_Partition_list **partition)
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


void HYDU_free_partition_list(struct HYD_Partition_list *partition)
{
    struct HYD_Partition_list *run, *p;
    int arg;

    HYDU_FUNC_ENTER();

    p = partition;
    run = p;
    while (run) {
        run = p->next;

        if (p->name) {
            HYDU_FREE(p->name);
            p->name = NULL;
        }

        if (p->mapping) {
            for (arg = 0; p->mapping[arg]; arg++) {
                HYDU_FREE(p->mapping[arg]);
                p->mapping[arg] = NULL;
            }
            HYDU_FREE(p->mapping);
            p->mapping = NULL;
        }

        HYDU_free_strlist(p->args);
        HYDU_FREE(p);

        p = run;
    }

    HYDU_FUNC_EXIT();
}
