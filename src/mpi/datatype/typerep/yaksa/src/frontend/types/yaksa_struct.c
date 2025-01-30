/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <assert.h>

int yaksi_type_create_struct(intptr_t count, const intptr_t * array_of_blocklengths,
                             const intptr_t * array_of_displs, yaksi_type_s ** array_of_intypes,
                             yaksi_type_s ** newtype)
{
    int rc = YAKSA_SUCCESS;

    /* shortcut for hindexed types */
    bool is_hindexed = true;
    for (intptr_t i = 1; i < count; i++) {
        if (array_of_intypes[i] != array_of_intypes[i - 1])
            is_hindexed = false;
    }
    if (is_hindexed) {
        rc = yaksi_type_create_hindexed(count, array_of_blocklengths, array_of_displs,
                                        array_of_intypes[0], newtype);
        YAKSU_ERR_CHECK(rc, fn_fail);
        goto fn_exit;
    }

    /* regular struct type */
    yaksi_type_s *outtype;
    outtype = (yaksi_type_s *) malloc(sizeof(yaksi_type_s));
    YAKSU_ERR_CHKANDJUMP(!outtype, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
    yaksu_atomic_store(&outtype->refcount, 1);

    outtype->kind = YAKSI_TYPE_KIND__STRUCT;

    outtype->size = 0;
    for (intptr_t i = 0; i < count; i++) {
        outtype->size += array_of_intypes[i]->size * array_of_blocklengths[i];
        yaksu_atomic_incr(&array_of_intypes[i]->refcount);
    }

    int is_set;
    intptr_t last_ub;
    int is_noncontig;
    is_set = 0;
    last_ub = 0;
    is_noncontig = 0;
    outtype->alignment = 0;
    for (intptr_t idx = 0; idx < count; idx++) {
        if (array_of_blocklengths[idx] == 0)
            continue;

        intptr_t lb, ub;
        if (array_of_intypes[idx]->extent > 0) {
            lb = array_of_displs[idx] + array_of_intypes[idx]->lb;
            ub = array_of_displs[idx] + array_of_intypes[idx]->ub +
                array_of_intypes[idx]->extent * (array_of_blocklengths[idx] - 1);
        } else {
            lb = array_of_displs[idx] + array_of_intypes[idx]->lb +
                array_of_intypes[idx]->extent * (array_of_blocklengths[idx] - 1);
            ub = array_of_displs[idx] + array_of_intypes[idx]->ub;
        }

        intptr_t true_lb = lb - array_of_intypes[idx]->lb + array_of_intypes[idx]->true_lb;
        intptr_t true_ub = ub - array_of_intypes[idx]->ub + array_of_intypes[idx]->true_ub;

        if (idx > 0 && true_lb != last_ub) {
            is_noncontig = 1;
        }
        last_ub = true_ub;

        int tree_depth = array_of_intypes[idx]->tree_depth;
        if (outtype->alignment < array_of_intypes[idx]->alignment)
            outtype->alignment = array_of_intypes[idx]->alignment;

        if (is_set) {
            outtype->lb = YAKSU_MIN(lb, outtype->lb);
            outtype->true_lb = YAKSU_MIN(true_lb, outtype->true_lb);
            outtype->ub = YAKSU_MAX(ub, outtype->ub);
            outtype->true_ub = YAKSU_MAX(true_ub, outtype->true_ub);
            outtype->tree_depth = YAKSU_MAX(tree_depth, outtype->tree_depth);
        } else {
            outtype->lb = lb;
            outtype->true_lb = true_lb;
            outtype->ub = ub;
            outtype->true_ub = true_ub;
            outtype->tree_depth = tree_depth;
        }

        is_set = 1;
    }

    /* adjust ub based on alignment */
    assert(outtype->alignment);
    uintptr_t diff;
    diff = (outtype->ub - outtype->lb) % outtype->alignment;
    if (diff) {
        outtype->ub += outtype->alignment - diff;
    }

    outtype->tree_depth++;
    outtype->extent = outtype->ub - outtype->lb;

    /* detect if the outtype is contiguous */
    if (!is_noncontig && (outtype->ub - outtype->lb) == outtype->size) {
        outtype->is_contig = true;

        for (intptr_t i = 0; i < count; i++) {
            if (array_of_intypes[i]->is_contig == false) {
                outtype->is_contig = false;
                break;
            }
        }
    } else {
        outtype->is_contig = false;
    }

    if (outtype->is_contig && outtype->ub == outtype->extent) {
        outtype->num_contig = 1;
    } else {
        outtype->num_contig = 0;
        for (intptr_t i = 0; i < count; i++) {
            if (array_of_intypes[i]->is_contig && array_of_blocklengths[i]) {
                outtype->num_contig++;
            } else {
                outtype->num_contig += array_of_intypes[i]->num_contig * array_of_blocklengths[i];
            }
        }
    }

    outtype->u.str.count = count;
    outtype->u.str.array_of_blocklengths = (intptr_t *) malloc(count * sizeof(intptr_t));
    outtype->u.str.array_of_displs = (intptr_t *) malloc(count * sizeof(intptr_t));
    outtype->u.str.array_of_types = (yaksi_type_s **) malloc(count * sizeof(yaksi_type_s *));
    for (intptr_t i = 0; i < count; i++) {
        outtype->u.str.array_of_blocklengths[i] = array_of_blocklengths[i];
        outtype->u.str.array_of_displs[i] = array_of_displs[i];
        outtype->u.str.array_of_types[i] = array_of_intypes[i];
    }

    rc = yaksur_type_create_hook(outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);
    *newtype = outtype;

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_type_create_struct(intptr_t count,
                                              const intptr_t * array_of_blocklengths,
                                              const intptr_t * array_of_displs,
                                              const yaksa_type_t * array_of_types,
                                              yaksa_info_t info, yaksa_type_t * newtype)
{
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    uintptr_t total_size;
    total_size = 0;
    for (intptr_t i = 0; i < count; i++) {
        yaksi_type_s *type;
        rc = yaksi_type_get(array_of_types[i], &type);
        YAKSU_ERR_CHECK(rc, fn_fail);

        total_size += type->size * array_of_blocklengths[i];
    }
    if (total_size == 0) {
        *newtype = YAKSA_TYPE__NULL;
        goto fn_exit;
    }

    assert(count > 0);
    yaksi_type_s **array_of_intypes;
    array_of_intypes = (yaksi_type_s **) malloc(count * sizeof(yaksi_type_s *));

    for (intptr_t i = 0; i < count; i++) {
        rc = yaksi_type_get(array_of_types[i], &array_of_intypes[i]);
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

    yaksi_type_s *outtype;
    rc = yaksi_type_create_struct(count, array_of_blocklengths, array_of_displs, array_of_intypes,
                                  &outtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_type_handle_alloc(outtype, newtype);
    YAKSU_ERR_CHECK(rc, fn_fail);

    free(array_of_intypes);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
