/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>

int yaksi_type_create_hindexed_block(intptr_t count, intptr_t blocklength,
                                     const intptr_t * array_of_displs,
                                     yaksi_type_s * intype, yaksi_type_s ** newtype)
{
    int rc = YAKSA_SUCCESS;

    /* shortcut for vector types */
    bool is_hvector = true;
    if (array_of_displs[0])
        is_hvector = false;
    for (int i = 2; i < count; i++) {
        if (array_of_displs[i] - array_of_displs[i - 1] != array_of_displs[1] - array_of_displs[0])
            is_hvector = false;
    }
    if (is_hvector) {
        if (count > 1) {
            rc = yaksi_type_create_hvector(count, blocklength,
                                           array_of_displs[1] - array_of_displs[0], intype,
                                           newtype);
            YAKSU_ERR_CHECK(rc, fn_fail);
        } else {
            rc = yaksi_type_create_hvector(count, blocklength, 0, intype, newtype);
            YAKSU_ERR_CHECK(rc, fn_fail);
        }
        goto fn_exit;
    }

    /* regular hindexed type */
    yaksi_type_s *outtype;
    outtype = (yaksi_type_s *) malloc(sizeof(yaksi_type_s));
    YAKSU_ERR_CHKANDJUMP(!outtype, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    yaksu_atomic_store(&outtype->refcount, 1);

    yaksu_atomic_incr(&intype->refcount);

    outtype->kind = YAKSI_TYPE_KIND__BLKHINDX;
    outtype->tree_depth = intype->tree_depth + 1;
    outtype->size = intype->size * blocklength * count;
    outtype->alignment = intype->alignment;

    intptr_t min_disp;
    min_disp = array_of_displs[0];
    intptr_t max_disp;
    max_disp = array_of_displs[0];
    for (int i = 1; i < count; i++) {
        if (array_of_displs[i] < min_disp)
            min_disp = array_of_displs[i];
        if (array_of_displs[i] > max_disp)
            max_disp = array_of_displs[i];
    }

    if (intype->extent > 0) {
        outtype->lb = min_disp + intype->lb;
        outtype->ub = max_disp + intype->ub + intype->extent * (blocklength - 1);
    } else {
        outtype->lb = min_disp + intype->lb + intype->extent * (blocklength - 1);
        outtype->ub = max_disp + intype->ub;
    }

    outtype->true_lb = outtype->lb + intype->true_lb - intype->lb;
    outtype->true_ub = outtype->ub - intype->ub + intype->true_ub;
    outtype->extent = outtype->ub - outtype->lb;

    /* detect if the outtype is contiguous */
    if (intype->is_contig && ((outtype->ub - outtype->lb) == outtype->size)) {
        outtype->is_contig = true;
        for (int i = 1; i < count; i++) {
            if (array_of_displs[i] != array_of_displs[i - 1] + intype->extent * blocklength) {
                outtype->is_contig = false;
                break;
            }
        }
    } else {
        outtype->is_contig = false;
    }

    if (outtype->is_contig) {
        outtype->num_contig = 1;
    } else if (intype->is_contig) {
        outtype->num_contig = intype->num_contig * count;
    } else {
        outtype->num_contig = intype->num_contig * count * blocklength;
    }

    outtype->u.blkhindx.count = count;
    outtype->u.blkhindx.blocklength = blocklength;
    outtype->u.blkhindx.array_of_displs = (intptr_t *) malloc(count * sizeof(intptr_t));
    for (int i = 0; i < count; i++)
        outtype->u.blkhindx.array_of_displs[i] = array_of_displs[i];
    outtype->u.blkhindx.child = intype;

    rc = yaksur_type_create_hook(outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    *newtype = outtype;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_type_create_hindexed_block(intptr_t count, intptr_t blocklength,
                                                      const intptr_t * array_of_displs,
                                                      yaksa_type_t oldtype, yaksa_info_t info,
                                                      yaksa_type_t * newtype)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    yaksi_type_s *intype;
    rc = yaksi_type_get(oldtype, &intype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (count * intype->size == 0) {
        *newtype = YAKSA_TYPE__NULL;
        goto fn_exit;
    }

    yaksi_type_s *outtype;
    rc = yaksi_type_create_hindexed_block(count, blocklength, array_of_displs, intype, &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_type_handle_alloc(outtype, newtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_type_create_indexed_block(intptr_t count, intptr_t blocklength,
                                                     const intptr_t * array_of_displs,
                                                     yaksa_type_t oldtype, yaksa_info_t info,
                                                     yaksa_type_t * newtype)
{
    intptr_t *real_array_of_displs = NULL;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    yaksi_type_s *intype;
    rc = yaksi_type_get(oldtype, &intype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    if (count * intype->size == 0) {
        *newtype = YAKSA_TYPE__NULL;
        goto fn_exit;
    }
    assert(count > 0);

    real_array_of_displs = (intptr_t *) malloc(count * sizeof(intptr_t));

    for (intptr_t i = 0; i < count; i++)
        real_array_of_displs[i] = array_of_displs[i] * intype->extent;

    yaksi_type_s *outtype;
    rc = yaksi_type_create_hindexed_block(count, blocklength, real_array_of_displs, intype,
                                          &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_type_handle_alloc(outtype, newtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    if (real_array_of_displs)
        free(real_array_of_displs);
    return rc;
  fn_fail:
    goto fn_exit;
}
