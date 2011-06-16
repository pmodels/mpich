/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "persist_client.h"

HYD_status HYDT_bsci_launcher_persist_init(void)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYDT_bsci_fns.launch_procs = HYDT_bscd_persist_launch_procs;
    HYDT_bsci_fns.wait_for_completion = HYDT_bscd_persist_wait_for_completion;

    HYDU_FUNC_EXIT();

    return status;
}
