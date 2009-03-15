/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "bsci.h"
#include "bscu.h"

struct HYD_BSCI_fns HYD_BSCI_fns;

HYD_Status HYD_BSCI_finalize(void)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYD_BSCI_fns.finalize();

    HYDU_FUNC_EXIT();

    return status;
}
