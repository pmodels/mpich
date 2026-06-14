/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bsci_launcher_manual_init(void)
{
    HYDT_bsci_fns.launch_procs = HYDT_bscd_common_launch_procs;

    return HYD_SUCCESS;
}
