/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"


/* FIXME: This should probably be added as a default function allowing
 * the bootstrap server to override if needed. */
HYD_Status HYD_BSCI_Get_universe_size(int *size)
{
    HYD_Status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    *size = -1;

    HYDU_FUNC_EXIT();
    return status;
}
