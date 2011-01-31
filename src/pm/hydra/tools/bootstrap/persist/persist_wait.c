/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "persist_client.h"

HYD_status HYDT_bscd_persist_wait_for_completion(int timeout)
{
    int ret, i, all_done;
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    while (1) {
        status = HYDT_dmx_wait_for_event(timeout);
        HYDU_ERR_POP(status, "error waiting for event\n");

        all_done = 1;
        for (i = 0; i < HYDT_bscd_persist_node_count; i++) {
            ret = HYDT_dmx_query_fd_registration(HYDT_bscd_persist_control_fd[i]);
            if (ret)
                all_done = 0;
        }

        if (all_done)
            break;
    }

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
