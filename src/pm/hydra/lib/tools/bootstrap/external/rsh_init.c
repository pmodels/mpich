/*
 * Copyright 2026 Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hydra.h"
#include "bsci.h"
#include "common.h"

HYD_status HYDT_bsci_launcher_rsh_init(void)
{
    HYDT_bsci_fns.query_env_inherit = HYDT_bscd_ssh_query_env_inherit;
    HYDT_bsci_fns.launch_procs = HYDT_bscd_common_launch_procs;

    return HYD_SUCCESS;
}
