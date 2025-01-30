/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>
#include <string.h>

static int flatten(yaksi_type_s * type, void *flattened_type)
{
    int rc = YAKSA_SUCCESS;
    char *flatbuf = (char *) flattened_type;

    /* copy this type */
    memcpy(flatbuf, type, sizeof(yaksi_type_s));
    flatbuf += sizeof(yaksi_type_s);

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            rc = flatten(type->u.contig.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__DUP:
            rc = flatten(type->u.dup.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = flatten(type->u.resized.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            rc = flatten(type->u.hvector.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            memcpy(flatbuf, type->u.blkhindx.array_of_displs,
                   type->u.blkhindx.count * sizeof(intptr_t));
            flatbuf += type->u.blkhindx.count * sizeof(intptr_t);

            rc = flatten(type->u.blkhindx.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            memcpy(flatbuf, type->u.hindexed.array_of_blocklengths,
                   type->u.hindexed.count * sizeof(intptr_t));
            flatbuf += type->u.hindexed.count * sizeof(intptr_t);

            memcpy(flatbuf, type->u.hindexed.array_of_displs,
                   type->u.hindexed.count * sizeof(intptr_t));
            flatbuf += type->u.hindexed.count * sizeof(intptr_t);

            rc = flatten(type->u.hindexed.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            memcpy(flatbuf, type->u.str.array_of_blocklengths,
                   type->u.str.count * sizeof(intptr_t));
            flatbuf += type->u.str.count * sizeof(intptr_t);

            memcpy(flatbuf, type->u.str.array_of_displs, type->u.str.count * sizeof(intptr_t));
            flatbuf += type->u.str.count * sizeof(intptr_t);

            for (intptr_t i = 0; i < type->u.str.count; i++) {
                rc = flatten(type->u.str.array_of_types[i], flatbuf);
                YAKSU_ERR_CHECK(rc, fn_fail);

                uintptr_t tmp;
                rc = yaksi_flatten_size(type->u.str.array_of_types[i], &tmp);
                YAKSU_ERR_CHECK(rc, fn_fail);

                flatbuf += tmp;
            }
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            rc = flatten(type->u.subarray.primary, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        default:
            assert(0);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_flatten(yaksa_type_t type, void *flattened_type)
{
    int rc = YAKSA_SUCCESS;
    yaksi_type_s *yaksi_type;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = flatten(yaksi_type, flattened_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
