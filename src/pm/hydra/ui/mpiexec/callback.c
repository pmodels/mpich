/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "hydra.h"
#include "hydra_utils.h"
#include "mpiexec.h"
#include "demux.h"

HYD_status HYD_uii_mpx_stdout_cb(void *buf, int buflen)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_write(STDOUT_FILENO, buf, buflen);
    HYDU_ERR_POP(status, "unable to write data to stdout\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

HYD_status HYD_uii_mpx_stderr_cb(void *buf, int buflen)
{
    HYD_status status = HYD_SUCCESS;

    HYDU_FUNC_ENTER();

    status = HYDU_sock_write(STDERR_FILENO, buf, buflen);
    HYDU_ERR_POP(status, "unable to write data to stdout\n");

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}
