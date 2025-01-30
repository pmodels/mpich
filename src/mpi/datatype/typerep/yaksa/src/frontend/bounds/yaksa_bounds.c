/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksa.h"
#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>

YAKSA_API_PUBLIC int yaksa_type_get_size(yaksa_type_t type, uintptr_t * size)
{
    yaksi_type_s *yaksi_type;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *size = yaksi_type->size;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_type_get_extent(yaksa_type_t type, intptr_t * lb, intptr_t * extent)
{
    yaksi_type_s *yaksi_type;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *lb = yaksi_type->lb;
    *extent = yaksi_type->extent;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_type_get_true_extent(yaksa_type_t type, intptr_t * lb, intptr_t * extent)
{
    yaksi_type_s *yaksi_type;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *lb = yaksi_type->true_lb;
    *extent = yaksi_type->true_ub - yaksi_type->true_lb;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
