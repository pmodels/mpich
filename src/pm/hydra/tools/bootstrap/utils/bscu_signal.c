/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_utils.h"
#include "bscu.h"

HYD_status HYDT_bscu_propagate_signal(int signum)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    for (i = 0; i < HYD_bscu_pid_count; i++)
        if (HYD_bscu_pid_list[i] != -1)
            kill(HYD_bscu_pid_list[i], signum);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
