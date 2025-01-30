/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>

int yaksi_type_create_resized(yaksi_type_s * intype, intptr_t lb, intptr_t extent,
                              yaksi_type_s ** newtype)
{
    int rc = YAKSA_SUCCESS;

    if (lb == intype->lb && extent == intype->extent) {
        rc = yaksi_type_create_dup(intype, newtype);
        YAKSU_ERR_CHECK(rc, fn_fail);
        goto fn_exit;
    }

    yaksi_type_s *outtype;
    outtype = (yaksi_type_s *) malloc(sizeof(yaksi_type_s));
    YAKSU_ERR_CHKANDJUMP(!outtype, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    yaksu_atomic_store(&outtype->refcount, 1);

    yaksu_atomic_incr(&intype->refcount);

    outtype->kind = YAKSI_TYPE_KIND__RESIZED;
    outtype->tree_depth = intype->tree_depth + 1;
    outtype->size = intype->size;
    outtype->alignment = intype->alignment;

    outtype->lb = lb;
    outtype->ub = lb + extent;
    outtype->true_lb = intype->true_lb;
    outtype->true_ub = intype->true_ub;
    outtype->extent = outtype->ub - outtype->lb;

    /* detect if the outtype is contiguous */
    if (intype->is_contig && ((outtype->ub - outtype->lb) == outtype->size)) {
        outtype->is_contig = true;
    } else {
        outtype->is_contig = false;
    }

    outtype->num_contig = intype->num_contig;

    outtype->u.resized.child = intype;

    rc = yaksur_type_create_hook(outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);
    *newtype = outtype;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_type_create_resized(yaksa_type_t oldtype, intptr_t lb, intptr_t extent,
                                               yaksa_info_t info, yaksa_type_t * newtype)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    yaksi_type_s *intype;
    rc = yaksi_type_get(oldtype, &intype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    yaksi_type_s *outtype;
    rc = yaksi_type_create_resized(intype, lb, extent, &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_type_handle_alloc(outtype, newtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
