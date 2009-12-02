/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra_base.h"
#include "hydra_utils.h"
#include "demux.h"

int main(int argc, char **argv)
{
    HYD_status status = HYD_SUCCESS;

    status = HYDU_dbg_init("hydserv");
    HYDU_ERR_POP(status, "unable to initialization debugging\n");

  fn_exit:
    if (status != HYD_SUCCESS)
        return -1;
    else
        return 0;

  fn_fail:
    goto fn_exit;
}
