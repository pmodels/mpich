/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
