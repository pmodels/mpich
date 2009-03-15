/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "bsci.h"
#include "bscu.h"

struct HYD_BSCI_fns HYD_BSCI_fns;

HYD_Status HYD_BSCI_init(char * bootstrap)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    HYD_BSCI_fns.launch_procs = NULL;
    HYD_BSCI_fns.get_usize = NULL;
    HYD_BSCI_fns.wait_for_completion = NULL;
    HYD_BSCI_fns.finalize = NULL;

    if (!strcmp(bootstrap, "ssh"))
        HYD_BSCI_ssh_init();
    else if (!strcmp(bootstrap, "fork"))
        HYD_BSCI_fork_init();
    else {
        HYDU_Error_printf("unrecognized bootstrap server: %s\n", bootstrap);
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }

    if (HYD_BSCI_fns.launch_procs == NULL) {
        /* This function is mandatory */
        HYDU_Error_printf("Mandatory bootstrap launch function undefined\n");
        status = HYD_INTERNAL_ERROR;
        goto fn_fail;
    }
    if (HYD_BSCI_fns.get_usize == NULL)
        HYD_BSCI_fns.get_usize = HYD_BSCU_get_usize;
    if (HYD_BSCI_fns.wait_for_completion == NULL)
        HYD_BSCI_fns.wait_for_completion = HYD_BSCU_wait_for_completion;
    if (HYD_BSCI_fns.finalize == NULL)
        HYD_BSCI_fns.finalize = HYD_BSCU_finalize;

fn_exit:
    HYDU_FUNC_EXIT();
    return status;

fn_fail:
    goto fn_exit;
}
