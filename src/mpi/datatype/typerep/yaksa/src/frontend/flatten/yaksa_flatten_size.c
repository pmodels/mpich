/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>

int yaksi_flatten_size(yaksi_type_s * type, uintptr_t * flattened_type_size)
{
    int rc = YAKSA_SUCCESS;
    uintptr_t tmp;

    *flattened_type_size = sizeof(yaksi_type_s);

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            rc = yaksi_flatten_size(type->u.contig.child, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        case YAKSI_TYPE_KIND__DUP:
            rc = yaksi_flatten_size(type->u.dup.child, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = yaksi_flatten_size(type->u.resized.child, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            rc = yaksi_flatten_size(type->u.hvector.child, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            /* add space for array_of_displs */
            *flattened_type_size += type->u.blkhindx.count * sizeof(intptr_t);

            rc = yaksi_flatten_size(type->u.blkhindx.child, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            /* add space for array_of_blocklengths */
            *flattened_type_size += type->u.hindexed.count * sizeof(intptr_t);
            /* add space for array_of_displs */
            *flattened_type_size += type->u.hindexed.count * sizeof(intptr_t);

            rc = yaksi_flatten_size(type->u.hindexed.child, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            /* add space for array_of_blocklengths */
            *flattened_type_size += type->u.str.count * sizeof(intptr_t);
            /* add space for array_of_displs */
            *flattened_type_size += type->u.str.count * sizeof(intptr_t);

            for (int i = 0; i < type->u.str.count; i++) {
                rc = yaksi_flatten_size(type->u.str.array_of_types[i], &tmp);
                YAKSU_ERR_CHECK(rc, fn_fail);
                *flattened_type_size += tmp;
            }
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            rc = yaksi_flatten_size(type->u.subarray.primary, &tmp);
            YAKSU_ERR_CHECK(rc, fn_fail);
            *flattened_type_size += tmp;
            break;

        default:
            assert(0);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_flatten_size(yaksa_type_t type, uintptr_t * flattened_type_size)
{
    int rc = YAKSA_SUCCESS;
    yaksi_type_s *yaksi_type;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_flatten_size(yaksi_type, flattened_type_size);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
