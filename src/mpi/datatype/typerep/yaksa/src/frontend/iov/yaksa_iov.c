/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "yaksi.h"
#include "yaksu.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* we always "pack" full elements into the IOV, never partial */
#define BUILTIN_PAIRTYPE_IOV(buf, count, iov_offset, max_iov_len, iov, TYPE, TYPE1, TYPE2, idx) \
    do {                                                                \
        TYPE tmp;                                                       \
        bool element_is_contig;                                         \
        uintptr_t offset = (const char *) &tmp.y - (const char *) &tmp; \
                                                                        \
        if (offset == sizeof(TYPE1))                                    \
            element_is_contig = true;                                   \
        else                                                            \
            element_is_contig = false;                                  \
                                                                        \
        idx = 0;                                                        \
        if (element_is_contig) {                                        \
            const char *z = (const char *) buf + (iov_offset * sizeof(TYPE)); \
            uintptr_t real_count = YAKSU_MIN(max_iov_len, count - iov_offset); \
            for (int x = 0; x < real_count; x++) {                      \
                iov[x].iov_base = (void *) z;                           \
                iov[x].iov_len = sizeof(TYPE1) + sizeof(TYPE2);         \
                z += sizeof(TYPE);                                      \
                (idx)++;                                                \
            }                                                           \
        } else {                                                        \
            assert(iov_offset % 2 == 0);                                \
            const char *z = (const char *) buf + (iov_offset * sizeof(TYPE) / 2); \
            uintptr_t real_count = YAKSU_MIN(max_iov_len - 1, count * 2 - iov_offset); \
            for (int x = 0; x < real_count; x += 2) {                   \
                iov[x].iov_base = (void *) z;                           \
                iov[x].iov_len = sizeof(TYPE1);                         \
                iov[x+1].iov_base = (void *) (z + offset);              \
                iov[x+1].iov_len = sizeof(TYPE2);                       \
                z += sizeof(TYPE);                                      \
                (idx) += 2;                                             \
            }                                                           \
        }                                                               \
    } while (0)

int yaksi_iov(const char *buf, uintptr_t count, yaksi_type_s * type, uintptr_t iov_offset,
              struct iovec *iov, uintptr_t max_iov_len, uintptr_t * actual_iov_len)
{
    int rc = YAKSA_SUCCESS;

    /* if the user didn't give any space to provide an iov, return */
    if (max_iov_len == 0) {
        *actual_iov_len = 0;
        goto fn_exit;
    }

    /* fast path for the contiguous case */
    if (type->is_contig) {
        /* unfortunately, struct iovec uses "char *" instead of "const
         * char *" because the same structure is used in readv calls
         * too, where the buffer is modified */
        iov[0].iov_base = (char *) buf + type->true_lb;
        iov[0].iov_len = count * type->size;
        *actual_iov_len = 1;
        goto fn_exit;
    }

    /* if the offset doesn't give any space to provide an iov,
     * return */
    if (iov_offset >= count * type->num_contig) {
        *actual_iov_len = 0;
        goto fn_exit;
    }

    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            {
                switch (type->u.builtin.handle) {
                    case YAKSA_TYPE__FLOAT_INT:
                        BUILTIN_PAIRTYPE_IOV(buf, count, iov_offset, max_iov_len, iov,
                                             yaksi_float_int_s, float, int, *actual_iov_len);
                        break;

                    case YAKSA_TYPE__DOUBLE_INT:
                        BUILTIN_PAIRTYPE_IOV(buf, count, iov_offset, max_iov_len, iov,
                                             yaksi_double_int_s, double, int, *actual_iov_len);
                        break;

                    case YAKSA_TYPE__LONG_INT:
                        BUILTIN_PAIRTYPE_IOV(buf, count, iov_offset, max_iov_len, iov,
                                             yaksi_long_int_s, long, int, *actual_iov_len);
                        break;

                    case YAKSA_TYPE__SHORT_INT:
                        BUILTIN_PAIRTYPE_IOV(buf, count, iov_offset, max_iov_len, iov,
                                             yaksi_short_int_s, short, int, *actual_iov_len);
                        break;

                    case YAKSA_TYPE__LONG_DOUBLE_INT:
                        BUILTIN_PAIRTYPE_IOV(buf, count, iov_offset, max_iov_len, iov,
                                             yaksi_long_double_int_s, long double, int,
                                             *actual_iov_len);
                        break;

                    default:
                        assert(0);
                }
            }
            break;

        case YAKSI_TYPE_KIND__HVECTOR:
            {
                const char *rem_buf = (const char *) buf;
                const char *type_start = (const char *) buf;
                struct iovec *rem_iov = iov;
                uintptr_t rem_iov_len = max_iov_len;
                uintptr_t rem_iov_offset = iov_offset;

                *actual_iov_len = 0;
                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.hvector.count; j++) {
                        uintptr_t num_contig;
                        yaksi_iov_len(type->u.hvector.blocklength, type->u.hvector.child,
                                      &num_contig);

                        if (rem_iov_offset >= num_contig) {
                            rem_iov_offset -= num_contig;
                        } else {
                            uintptr_t tmp_iov_len;

                            rc = yaksi_iov(rem_buf, type->u.hvector.blocklength,
                                           type->u.hvector.child, rem_iov_offset, rem_iov,
                                           rem_iov_len, &tmp_iov_len);
                            YAKSU_ERR_CHECK(rc, fn_fail);

                            rem_iov_len -= tmp_iov_len;
                            rem_iov += tmp_iov_len;
                            *actual_iov_len += tmp_iov_len;
                            rem_iov_offset = 0;

                            if (rem_iov_len == 0)
                                goto fn_exit;
                        }

                        rem_buf += type->u.hvector.stride;
                    }

                    type_start += type->extent;
                    rem_buf = type_start;
                }
            }
            break;

        case YAKSI_TYPE_KIND__BLKHINDX:
            {
                struct iovec *rem_iov = iov;
                uintptr_t rem_iov_len = max_iov_len;
                uintptr_t rem_iov_offset = iov_offset;

                *actual_iov_len = 0;
                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.blkhindx.count; j++) {
                        uintptr_t num_contig;
                        yaksi_iov_len(type->u.blkhindx.blocklength, type->u.blkhindx.child,
                                      &num_contig);

                        if (rem_iov_offset >= num_contig) {
                            rem_iov_offset -= num_contig;
                        } else {
                            const char *rem_buf =
                                (const char *) buf + i * type->extent +
                                type->u.blkhindx.array_of_displs[j];
                            uintptr_t tmp_iov_len;

                            rc = yaksi_iov(rem_buf, type->u.blkhindx.blocklength,
                                           type->u.blkhindx.child, rem_iov_offset, rem_iov,
                                           rem_iov_len, &tmp_iov_len);
                            YAKSU_ERR_CHECK(rc, fn_fail);

                            rem_iov_len -= tmp_iov_len;
                            rem_iov += tmp_iov_len;
                            *actual_iov_len += tmp_iov_len;
                            rem_iov_offset = 0;

                            if (rem_iov_len == 0)
                                goto fn_exit;
                        }
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__HINDEXED:
            {
                struct iovec *rem_iov = iov;
                uintptr_t rem_iov_len = max_iov_len;
                uintptr_t rem_iov_offset = iov_offset;

                *actual_iov_len = 0;
                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.hindexed.count; j++) {
                        if (type->u.hindexed.array_of_blocklengths[j] == 0)
                            continue;

                        uintptr_t num_contig;
                        yaksi_iov_len(type->u.hindexed.array_of_blocklengths[j],
                                      type->u.hindexed.child, &num_contig);

                        if (rem_iov_offset >= num_contig) {
                            rem_iov_offset -= num_contig;
                        } else {
                            const char *rem_buf =
                                (const char *) buf + i * type->extent +
                                type->u.hindexed.array_of_displs[j];
                            uintptr_t tmp_iov_len;

                            rc = yaksi_iov(rem_buf, type->u.hindexed.array_of_blocklengths[j],
                                           type->u.hindexed.child, rem_iov_offset, rem_iov,
                                           rem_iov_len, &tmp_iov_len);
                            YAKSU_ERR_CHECK(rc, fn_fail);

                            rem_iov_len -= tmp_iov_len;
                            rem_iov += tmp_iov_len;
                            *actual_iov_len += tmp_iov_len;
                            rem_iov_offset = 0;

                            if (rem_iov_len == 0)
                                goto fn_exit;
                        }
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__STRUCT:
            {
                struct iovec *rem_iov = iov;
                uintptr_t rem_iov_len = max_iov_len;
                uintptr_t rem_iov_offset = iov_offset;

                *actual_iov_len = 0;
                for (int i = 0; i < count; i++) {
                    for (int j = 0; j < type->u.str.count; j++) {
                        if (type->u.str.array_of_blocklengths[j] == 0)
                            continue;

                        uintptr_t num_contig;
                        yaksi_iov_len(type->u.str.array_of_blocklengths[j],
                                      type->u.str.array_of_types[j], &num_contig);

                        if (rem_iov_offset >= num_contig) {
                            rem_iov_offset -= num_contig;
                        } else {
                            const char *rem_buf =
                                (const char *) buf + i * type->extent +
                                type->u.str.array_of_displs[j];
                            uintptr_t tmp_iov_len;

                            rc = yaksi_iov(rem_buf, type->u.str.array_of_blocklengths[j],
                                           type->u.str.array_of_types[j], rem_iov_offset, rem_iov,
                                           rem_iov_len, &tmp_iov_len);
                            YAKSU_ERR_CHECK(rc, fn_fail);

                            rem_iov_len -= tmp_iov_len;
                            rem_iov += tmp_iov_len;
                            *actual_iov_len += tmp_iov_len;
                            rem_iov_offset = 0;

                            if (rem_iov_len == 0)
                                goto fn_exit;
                        }
                    }
                }
            }
            break;

        case YAKSI_TYPE_KIND__RESIZED:
            {
                const char *rem_buf = (const char *) buf;
                struct iovec *rem_iov = iov;
                uintptr_t rem_iov_len = max_iov_len;
                uintptr_t rem_iov_offset = iov_offset;

                *actual_iov_len = 0;
                for (int i = 0; i < count; i++) {
                    uintptr_t num_contig;
                    yaksi_iov_len(1, type->u.resized.child, &num_contig);

                    if (rem_iov_offset >= num_contig) {
                        rem_iov_offset -= num_contig;
                    } else {
                        uintptr_t tmp_iov_len;

                        rc = yaksi_iov(rem_buf, 1, type->u.resized.child, rem_iov_offset, rem_iov,
                                       rem_iov_len, &tmp_iov_len);
                        YAKSU_ERR_CHECK(rc, fn_fail);

                        rem_iov_len -= tmp_iov_len;
                        rem_iov += tmp_iov_len;
                        *actual_iov_len += tmp_iov_len;
                        rem_iov_offset = 0;

                        if (rem_iov_len == 0)
                            goto fn_exit;
                    }

                    rem_buf += type->extent;
                }
            }
            break;

        case YAKSI_TYPE_KIND__CONTIG:
            {
                rc = yaksi_iov(buf, count * type->u.contig.count, type->u.contig.child,
                               iov_offset, iov, max_iov_len, actual_iov_len);
                YAKSU_ERR_CHECK(rc, fn_fail);
            }
            break;

        case YAKSI_TYPE_KIND__SUBARRAY:
            {
                yaksi_type_s *primary = type->u.subarray.primary;
                const char *rem_buf = (const char *) buf + type->true_lb - primary->true_lb;
                struct iovec *rem_iov = iov;
                uintptr_t rem_iov_len = max_iov_len;
                uintptr_t rem_iov_offset = iov_offset;

                *actual_iov_len = 0;
                for (int i = 0; i < count; i++) {
                    uintptr_t num_contig;
                    yaksi_iov_len(1, primary, &num_contig);

                    if (rem_iov_offset >= num_contig) {
                        rem_iov_offset -= num_contig;
                    } else {
                        uintptr_t tmp_iov_len;

                        rc = yaksi_iov(rem_buf, 1, primary, rem_iov_offset, rem_iov, rem_iov_len,
                                       &tmp_iov_len);
                        YAKSU_ERR_CHECK(rc, fn_fail);

                        rem_iov_len -= tmp_iov_len;
                        rem_iov += tmp_iov_len;
                        *actual_iov_len += tmp_iov_len;
                        rem_iov_offset = 0;
                    }

                    rem_buf += type->extent;
                }
            }
            break;

        case YAKSI_TYPE_KIND__DUP:
            rc = yaksi_iov(buf, count, type->u.dup.child, iov_offset, iov, max_iov_len,
                           actual_iov_len);
            YAKSU_ERR_CHECK(rc, fn_fail);
            break;
    }

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}

YAKSA_API_PUBLIC int yaksa_iov(const char *buf, uintptr_t count, yaksa_type_t type,
                               uintptr_t iov_offset, struct iovec *iov, uintptr_t max_iov_len,
                               uintptr_t * actual_iov_len)
{
    yaksi_type_s *yaksi_type;
    int rc = YAKSA_SUCCESS;

    assert(yaksu_atomic_load(&yaksi_is_initialized));

    rc = yaksi_type_get(type, &yaksi_type);
    YAKSU_ERR_CHECK(rc, fn_fail);

    rc = yaksi_iov(buf, count, yaksi_type, iov_offset, iov, max_iov_len, actual_iov_len);
    YAKSU_ERR_CHECK(rc, fn_fail);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
