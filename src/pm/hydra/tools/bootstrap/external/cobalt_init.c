/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "bsci.h"
#include "cobalt.h"

HYD_status HYDT_bsci_rmk_cobalt_init(void)
{
    HYDT_bsci_fns.query_node_list = HYDT_bscd_cobalt_query_node_list;
    HYDT_bsci_fns.query_native_int = HYDT_bscd_cobalt_query_native_int;

    return HYD_SUCCESS;
}
