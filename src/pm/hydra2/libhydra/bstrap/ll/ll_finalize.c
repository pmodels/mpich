/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "hydra_bstrap_ll.h"
#include "hydra_err.h"

HYD_status HYDI_bstrap_ll_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
