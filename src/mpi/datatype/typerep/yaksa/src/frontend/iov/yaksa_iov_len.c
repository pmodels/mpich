/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int yaksi_iov_len(uintptr_t count, yaksi_type_s * type, uintptr_t * iov_len)
{
    if (type->is_contig) {
        *iov_len = 1;
    } else {
        *iov_len = count * type->num_contig;
    }

    return YAKSA_SUCCESS;
}

YAKSA_API_PUBLIC int yaksa_iov_len(uintptr_t count, yaksa_type_t type, uintptr_t * iov_len)
{
    yaksi_type_s *yaksi_type;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_iov_len(count, yaksi_type, iov_len);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
