/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <assert.h>

static int yaksi_iunpack_internal(const void *inbuf, void *outbuf, uintptr_t count,
                                  yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                  yaksi_request_s * request);

#define BUILTIN_PAIRTYPE_UNPACK(type, TYPE1, TYPE2, inbuf, outbuf, count, info, op, request) \
    do {                                                                \
        type z;                                                         \
        uintptr_t offset = (char *) &z.y - (char *) &z;                 \
        const char *sbuf = (const char *) inbuf;                        \
        char *dbuf = (char *) outbuf;                                   \
                                                                        \
        yaksi_type_s *type1;                                            \
        rc = yaksi_type_get(YAKSA_TYPE__ ## TYPE1, &type1);             \
        YAKSU_ERR_CHECK(rc, fn_fail);                                   \
                                                                        \
        yaksi_type_s *type2;                                            \
        rc = yaksi_type_get(YAKSA_TYPE__ ## TYPE2, &type2);             \
        YAKSU_ERR_CHECK(rc, fn_fail);                                   \
                                                                        \
        for (int i = 0; i < count; i++) {                               \
            rc = yaksi_iunpack_internal(sbuf, dbuf, 1, type1, info, op, request); \
            YAKSU_ERR_CHECK(rc, fn_fail);                               \
            sbuf += type1->size;                                        \
                                                                        \
            rc = yaksi_iunpack_internal(sbuf, dbuf + offset, 1, type2, info, op, request); \
            YAKSU_ERR_CHECK(rc, fn_fail);                               \
            sbuf += type2->size;                                        \
                                                                        \
            dbuf += sizeof(type);                                       \
        }                                                               \
    } while (0)

static inline int unpack_backend(const void *inbuf, void *outbuf, uintptr_t count,
                                 yaksi_type_s * type, yaksi_info_s * info,
                                 yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* always query the ptr attributes for datatypes with absolute addresses */
    if (!outbuf) {
        request->always_query_ptr_attr = true;
    }

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            switch (type->u.builtin.handle) {
                case YAKSA_TYPE__FLOAT_INT:
                    BUILTIN_PAIRTYPE_UNPACK(yaksi_float_int_s, FLOAT, INT, inbuf, outbuf, count,
                                            info, op, request);
                    break;

                case YAKSA_TYPE__DOUBLE_INT:
                    BUILTIN_PAIRTYPE_UNPACK(yaksi_double_int_s, DOUBLE, INT, inbuf, outbuf, count,
                                            info, op, request);
                    break;

                case YAKSA_TYPE__LONG_INT:
                    BUILTIN_PAIRTYPE_UNPACK(yaksi_long_int_s, LONG, INT, inbuf, outbuf, count,
                                            info, op, request);
                    break;

                case YAKSA_TYPE__SHORT_INT:
                    BUILTIN_PAIRTYPE_UNPACK(yaksi_short_int_s, SHORT, INT, inbuf, outbuf, count,
                                            info, op, request);
                    break;

                case YAKSA_TYPE__LONG_DOUBLE_INT:
                    BUILTIN_PAIRTYPE_UNPACK(yaksi_long_double_int_s, LONG_DOUBLE, INT, inbuf,
                                            outbuf, count, info, op, request);
                    break;

                default:
                    assert(0);
            }
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            {
                const char *sbuf = (const char *) inbuf;
                char *dbuf = (char *) outbuf;
                char *type_start = (char *) outbuf;
                uintptr_t size = type->u.hvector.blocklength * type->u.hvector.child->size;

                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.hvector.count; j++) {
                        rc = yaksi_iunpack_internal(sbuf, dbuf, type->u.hvector.blocklength,
                                                    type->u.hvector.child, info, op, request);
                        YAKSU_ERR_CHECK(rc, fn_fail);
                        sbuf += size;
                        dbuf += type->u.hvector.stride;
                    }

                    type_start += type->extent;
                    dbuf = type_start;
                }
            }
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            {
                const char *sbuf = (const char *) inbuf;
                char *dbuf;
                uintptr_t size = type->u.blkhindx.blocklength * type->u.blkhindx.child->size;

                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.blkhindx.count; j++) {
                        dbuf =
                            (char *) outbuf + i * type->extent +
                            type->u.blkhindx.array_of_displs[j];
                        rc = yaksi_iunpack_internal(sbuf, dbuf, type->u.blkhindx.blocklength,
                                                    type->u.blkhindx.child, info, op, request);
                        YAKSU_ERR_CHECK(rc, fn_fail);
                        sbuf += size;
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            {
                const char *sbuf = (const char *) inbuf;
                char *dbuf;

                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.hindexed.count; j++) {
                        if (type->u.hindexed.array_of_blocklengths[j] == 0)
                            continue;

                        dbuf =
                            (char *) outbuf + i * type->extent +
                            type->u.hindexed.array_of_displs[j];
                        rc = yaksi_iunpack_internal(sbuf, dbuf,
                                                    type->u.hindexed.array_of_blocklengths[j],
                                                    type->u.hindexed.child, info, op, request);
                        YAKSU_ERR_CHECK(rc, fn_fail);
                        sbuf +=
                            type->u.hindexed.array_of_blocklengths[j] *
                            type->u.hindexed.child->size;
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            {
                const char *sbuf = (const char *) inbuf;
                char *dbuf;

                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.str.count; j++) {
                        if (type->u.str.array_of_blocklengths[j] == 0)
                            continue;

                        dbuf = (char *) outbuf + i * type->extent + type->u.str.array_of_displs[j];
                        rc = yaksi_iunpack_internal(sbuf, dbuf,
                                                    type->u.str.array_of_blocklengths[j],
                                                    type->u.str.array_of_types[j], info, op,
                                                    request);
                        YAKSU_ERR_CHECK(rc, fn_fail);
                        sbuf +=
                            type->u.str.array_of_blocklengths[j] *
                            type->u.str.array_of_types[j]->size;
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            {
                const char *sbuf = (const char *) inbuf;
                char *dbuf = (char *) outbuf;

                for (int i = 0; i < count; i++) {
                    rc = yaksi_iunpack_internal(sbuf, dbuf, 1, type->u.resized.child, info, op,
                                                request);
                    YAKSU_ERR_CHECK(rc, fn_fail);

                    sbuf += type->u.resized.child->size;
                    dbuf += type->extent;
                }
            }
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            {
                rc = yaksi_iunpack_internal(inbuf, outbuf, count * type->u.contig.count,
                                            type->u.contig.child, info, op, request);
                YAKSU_ERR_CHECK(rc, fn_fail);
            }
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            {
                char *dbuf = (char *) outbuf + type->true_lb - type->u.subarray.primary->true_lb;
                rc = yaksi_iunpack_internal(inbuf, dbuf, count, type->u.subarray.primary, info,
                                            op, request);
                YAKSU_ERR_CHECK(rc, fn_fail);
            }
            break;

        default:
            assert(0);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

int yaksi_iunpack_backend(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                          yaksi_info_s * info, yaksa_op_t op, yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    rc = (outbuf) ? yaksur_iunpack(inbuf, outbuf, count, type, info, op,
                                   request) : YAKSA_ERR__NOT_SUPPORTED;
    if (rc == YAKSA_ERR__NOT_SUPPORTED) {
        rc = unpack_backend(inbuf, outbuf, count, type, info, op, request);
        YAKSU_ERR_CHECK(rc, fn_fail);
    } else {
        YAKSU_ERR_CHECK(rc, fn_fail);
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

/* Essentially the same as yaksi_iunpack_backend.
 * This is to work-around a clang (v10 and v11) compiler bug that confuses the branches
 * in yaksi_iunpack_backend when the initial inbuf is NULL (MPI_BOTTOM).
 */
static int yaksi_iunpack_internal(const void *inbuf, void *outbuf, uintptr_t count,
                                  yaksi_type_s * type, yaksi_info_s * info, yaksa_op_t op,
                                  yaksi_request_s * request)
{
    int rc = YAKSA_SUCCESS;

    /* The compiler bug will trigger the assertion even though outbuf != NULL */
    /* assert(outbuf); */
    rc = yaksur_iunpack(inbuf, outbuf, count, type, info, op, request);
    if (rc == YAKSA_ERR__NOT_SUPPORTED) {
        rc = unpack_backend(inbuf, outbuf, count, type, info, op, request);
    }
    return rc;
}
