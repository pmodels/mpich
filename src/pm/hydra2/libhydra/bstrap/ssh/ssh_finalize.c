/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_bstrap_ssh.h"
#include "ssh_internal.h"
#include "hydra_err.h"

HYD_status HYDI_bstrap_ssh_finalize(void)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDI_ssh_free_launch_elements();
    HYD_ERR_POP(status, "error freeing launch elements\n");

  fn_exit:
    return status;

  fn_fail:
    goto fn_exit;
}
