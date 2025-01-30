/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <assert.h>
#include "yaksa.h"
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri.h"

int yaksur_request_test(yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    rc = yaksuri_progress_poke();
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksur_request_wait(yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    while (yaksu_atomic_load(&request->cc)) {
        rc = yaksuri_progress_poke();
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
