/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static inline int unflatten(yaksi_type_s ** type, const void *flattened_type)
{
    int rc = YAKSA_SUCCESS;
    yaksi_type_s *newtype = NULL;
    const char *flatbuf = (const char *) flattened_type;

    if (((yaksi_type_s *) flattened_type)->kind == YAKSI_TYPE_KIND__BUILTIN) {
        yaksa_type_t id = ((yaksi_type_s *) flattened_type)->u.builtin.handle;
        yaksi_type_s *tmp;
        rc = yaksi_type_get(id, &tmp);
        YAKSU_ERR_CHECK(rc, fn_fail);
        yaksu_atomic_incr(&tmp->refcount);
        newtype = tmp;
        goto fn_exit;
    } else {
        newtype = (yaksi_type_s *) malloc(sizeof(yaksi_type_s));
        YAKSU_ERR_CHKANDJUMP(!newtype, rc, YAKSA_ERR__OUT_OF_MEM, fn_fail);
        yaksu_atomic_store(&newtype->refcount, 1);

        memcpy(newtype, flatbuf, sizeof(yaksi_type_s));
        flatbuf += sizeof(yaksi_type_s);
        yaksu_atomic_store(&newtype->refcount, 1);
    }

    switch (newtype->kind) {
        case YAKSI_TYPE_KIND__CONTIG:
            rc = unflatten(&newtype->u.contig.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__DUP:
            rc = unflatten(&newtype->u.dup.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            rc = unflatten(&newtype->u.resized.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            rc = unflatten(&newtype->u.hvector.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            newtype->u.blkhindx.array_of_displs =
                (intptr_t *) malloc(newtype->u.blkhindx.count * sizeof(intptr_t));
            memcpy(newtype->u.blkhindx.array_of_displs, flatbuf,
                   newtype->u.blkhindx.count * sizeof(intptr_t));
            flatbuf += newtype->u.blkhindx.count * sizeof(intptr_t);

            rc = unflatten(&newtype->u.blkhindx.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            newtype->u.hindexed.array_of_blocklengths =
                (intptr_t *) malloc(newtype->u.hindexed.count * sizeof(intptr_t));
            memcpy(newtype->u.hindexed.array_of_blocklengths, flatbuf,
                   newtype->u.hindexed.count * sizeof(intptr_t));
            flatbuf += newtype->u.hindexed.count * sizeof(intptr_t);

            newtype->u.hindexed.array_of_displs =
                (intptr_t *) malloc(newtype->u.hindexed.count * sizeof(intptr_t));
            memcpy(newtype->u.hindexed.array_of_displs, flatbuf,
                   newtype->u.hindexed.count * sizeof(intptr_t));
            flatbuf += newtype->u.hindexed.count * sizeof(intptr_t);

            rc = unflatten(&newtype->u.hindexed.child, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            newtype->u.str.array_of_blocklengths =
                (intptr_t *) malloc(newtype->u.str.count * sizeof(intptr_t));
            memcpy(newtype->u.str.array_of_blocklengths, flatbuf,
                   newtype->u.str.count * sizeof(intptr_t));
            flatbuf += newtype->u.str.count * sizeof(intptr_t);

            newtype->u.str.array_of_displs =
                (intptr_t *) malloc(newtype->u.str.count * sizeof(intptr_t));
            memcpy(newtype->u.str.array_of_displs, flatbuf,
                   newtype->u.str.count * sizeof(intptr_t));
            flatbuf += newtype->u.str.count * sizeof(intptr_t);

            newtype->u.str.array_of_types =
                (yaksi_type_s **) malloc(newtype->u.str.count * sizeof(yaksi_type_s *));
            for (int i = 0; i < newtype->u.str.count; i++) {
                rc = unflatten(&newtype->u.str.array_of_types[i], flatbuf);
                YAKSU_ERR_CHECK(rc, fn_fail);

                uintptr_t tmp;
                rc = yaksi_flatten_size(newtype->u.str.array_of_types[i], &tmp);
                YAKSU_ERR_CHECK(rc, fn_fail);

                flatbuf += tmp;
            }
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            rc = unflatten(&newtype->u.subarray.primary, flatbuf);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;

        default:
            assert(0);
    }

    yaksur_type_create_hook(newtype);

  fn_exit:
    *type = newtype;
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_unflatten(yaksa_type_t * type, const void *flattened_type)
{
    int rc = YAKSA_SUCCESS;
    yaksi_type_s *yaksi_type;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = unflatten(&yaksi_type, flattened_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    assert(yaksi_type);

    rc = yaksi_type_handle_alloc(yaksi_type, type);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
