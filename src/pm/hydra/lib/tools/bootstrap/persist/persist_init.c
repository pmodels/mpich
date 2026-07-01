/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
