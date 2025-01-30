/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>

YAKSA_API_PUBLIC int yaksa_pack_stream(const void *inbuf, uintptr_t incount, yaksa_type_t type,
                                       uintptr_t inoffset, void *outbuf, uintptr_t max_pack_bytes,
                                       uintptr_t * actual_pack_bytes, yaksa_info_t info,
                                       yaksa_op_t op, void *stream)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    if (incount == 0) {
        *actual_pack_bytes = 0;
        goto fn_exit;
    }

    yaksi_type_s *yaksi_type;
    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (yaksi_type->size == 0) {
        *actual_pack_bytes = 0;
        goto fn_exit;
    }

    yaksi_request_s *yaksi_request;
    rc = yaksi_request_create(&yaksi_request);
    YAKSU_ERR_CHECK(rc, fn_fail);
    yaksi_request_set_stream(yaksi_request, stream);

    yaksi_info_s *yaksi_info;
    yaksi_info = (yaksi_info_s *) info;

    rc = yaksi_ipack(inbuf, incount, yaksi_type, inoffset, outbuf, max_pack_bytes,
                     actual_pack_bytes, yaksi_info, op, yaksi_request);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_request_free(yaksi_request);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
