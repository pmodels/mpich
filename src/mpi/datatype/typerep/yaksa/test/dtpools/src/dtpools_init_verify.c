/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "dtpools_internal.h"
#include <stdint.h>
#include <stddef.h>

#define MAX_ERRCOUNT 10

static int errcount = 0;

/* FIXME: the print format used is always converted to %d below */
#define INIT_VERIFY_SINGLE_VAL(rc_, buf_, val_, verify_, c_type)        \
    do {                                                                \
        c_type *b_ = (c_type *) (buf_);                                 \
        if (verify_) {                                                  \
            if (*b_ != (c_type) val_) {                                 \
                errcount++;                                             \
                if (errcount < MAX_ERRCOUNT)                            \
                    printf("expected %d at %p, but found %d\n",         \
                           (int) val_, buf_, (int) b_[0]);              \
                DTPI_ERR_SETANDJUMP(rc_, DTP_ERR_OTHER);                \
            }                                                           \
        } else {                                                        \
            *b_ = (c_type) val_;                                        \
        }                                                               \
    } while (0)

#define INIT_VERIFY_DOUBLE_VAL(rc_, buf_, val_1, val_2, verify_, c_type1, c_type2) \
    do {                                                                \
        struct c_type3 {                                                \
            c_type1 a;                                                  \
            c_type2 b;                                                  \
        };                                                              \
        struct c_type3 *b_ = (struct c_type3 *) (buf_);                 \
        if (verify_) {                                                  \
            if (b_->a != (c_type1) val_1 || b_->b != (c_type2) val_2) { \
                errcount++;                                             \
                if (errcount < MAX_ERRCOUNT)                            \
                    printf("expected (%d,%d) at %p, but found (%d,%d)\n", \
                           (int) val_1, (int) val_2, buf_, (int) b_[0].a, (int) b_[0].b); \
                DTPI_ERR_SETANDJUMP(rc_, DTP_ERR_OTHER);                \
            }                                                           \
        } else {                                                        \
            b_[0].a = (c_type1) val_1;                                  \
            b_[0].b = (c_type2) val_2;                                  \
        }                                                               \
    } while (0)

static int init_verify_basic_datatype(yaksa_type_t type_, char *buf, int *val_, int val_stride,
                                      int verify)
{
    yaksa_type_t type = type_;
    int val = *val_;
    int rc = DTP_SUCCESS;

    switch (type) {
        case YAKSA_TYPE___BOOL:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, _Bool);
            val += val_stride;
            break;
        case YAKSA_TYPE__CHAR:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, char);
            val += val_stride;
            break;
        case YAKSA_TYPE__SIGNED_CHAR:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, signed char);
            val += val_stride;
            break;
        case YAKSA_TYPE__UNSIGNED_CHAR:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, unsigned char);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int);
            val += val_stride;
            break;
        case YAKSA_TYPE__UNSIGNED:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, unsigned);
            val += val_stride;
            break;
        case YAKSA_TYPE__SHORT:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, short);
            val += val_stride;
            break;
        case YAKSA_TYPE__UNSIGNED_SHORT:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, unsigned short);
            val += val_stride;
            break;
        case YAKSA_TYPE__LONG:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, long);
            val += val_stride;
            break;
        case YAKSA_TYPE__UNSIGNED_LONG:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, unsigned long);
            val += val_stride;
            break;
        case YAKSA_TYPE__LONG_LONG:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, long long);
            val += val_stride;
            break;
        case YAKSA_TYPE__UNSIGNED_LONG_LONG:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, unsigned long long);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT8_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int8_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT16_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int16_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT32_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int32_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT64_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int64_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__BYTE:
        case YAKSA_TYPE__UINT8_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint8_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT16_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint16_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT32_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint32_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT64_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint64_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_FAST8_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_fast8_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_FAST16_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_fast16_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_FAST32_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_fast32_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_FAST64_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_fast64_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_FAST8_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_fast8_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_FAST16_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_fast16_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_FAST32_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_fast32_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_FAST64_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_fast64_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_LEAST8_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_least8_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_LEAST16_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_least16_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_LEAST32_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_least32_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INT_LEAST64_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, int_least64_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_LEAST8_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_least8_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_LEAST16_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_least16_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_LEAST32_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_least32_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINT_LEAST64_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uint_least64_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__INTMAX_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, intmax_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINTMAX_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uintmax_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__SIZE_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, size_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__FLOAT:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, float);
            val += val_stride;
            break;
        case YAKSA_TYPE__DOUBLE:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, double);
            val += val_stride;
            break;
        case YAKSA_TYPE__LONG_DOUBLE:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, long double);
            val += val_stride;
            break;
        case YAKSA_TYPE__INTPTR_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, intptr_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__UINTPTR_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, uintptr_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__PTRDIFF_T:
            INIT_VERIFY_SINGLE_VAL(rc, buf, val, verify, ptrdiff_t);
            val += val_stride;
            break;
        case YAKSA_TYPE__C_COMPLEX:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, float, float);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__C_DOUBLE_COMPLEX:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, double, double);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, long double,
                                   long double);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__FLOAT_INT:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, float, int);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__DOUBLE_INT:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, double, int);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__LONG_INT:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, long, int);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__2INT:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, int, int);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__SHORT_INT:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, short, int);
            val += (2 * val_stride);
            break;
        case YAKSA_TYPE__LONG_DOUBLE_INT:
            INIT_VERIFY_DOUBLE_VAL(rc, buf, val, val + val_stride, verify, long double, int);
            val += (2 * val_stride);
            break;
        default:
            DTPI_ERR_ASSERT(0, rc);
    }

    *val_ = val;

  fn_exit:
    return rc;

  fn_fail:
    goto fn_exit;
}

static int init_verify_base_type(DTP_pool_s dtp, DTP_obj_s obj, void *buf_,
                                 uintptr_t buf_offset, int *val_start, int val_stride, int verify)
{
    DTPI_pool_s *dtpi = dtp.priv;
    int val = *val_start;
    char *buf = (char *) buf_ + buf_offset;
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    if (dtpi->base_type_is_struct) {
        for (intptr_t i = 0; i < dtpi->base_type_attrs.numblks; i++) {
            intptr_t offset = dtpi->base_type_attrs.array_of_displs[i];
            for (int j = 0; j < dtpi->base_type_attrs.array_of_blklens[i]; j++) {
                rc = init_verify_basic_datatype(dtpi->base_type_attrs.array_of_types[i],
                                                buf + offset, &val, val_stride, verify);
                DTPI_ERR_CHK_RC(rc);

                uintptr_t size;
                rc = yaksa_type_get_size(dtpi->base_type_attrs.array_of_types[i], &size);
                DTPI_ERR_CHK_RC(rc);
                offset += size;
            }
        }
    } else {
        rc = init_verify_basic_datatype(dtp.DTP_base_type, buf, &val, val_stride, verify);
        DTPI_ERR_CHK_RC(rc);
    }

  fn_exit:
    *val_start = val;
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

static int init_verify_subarray(DTP_pool_s dtp, DTP_obj_s obj, void *buf, DTPI_Attr_s * attr_tree,
                                int ndims, uintptr_t buf_offset, int *val_start, int val_stride,
                                int *rem_val_count, int verify)
{
    int rc = DTP_SUCCESS;
    DTPI_Attr_s *attr = attr_tree;

    DTPI_FUNC_ENTER();

    DTPI_ERR_ASSERT(attr->kind == DTPI_DATATYPE_KIND__SUBARRAY, rc);

    if (ndims == 0) {
        rc = DTPI_init_verify(dtp, obj, buf, attr->child, buf_offset, val_start, val_stride,
                              rem_val_count, verify);
        DTPI_ERR_CHK_RC(rc);
        goto fn_exit;
    }

    if (attr->u.subarray.order == YAKSA_SUBARRAY_ORDER__C) {
        int dims_offset = attr->u.subarray.ndims - ndims;

        uintptr_t base_offset = 1;
        for (int i = dims_offset + 1; i < attr->u.subarray.ndims; i++)
            base_offset *= attr->u.subarray.array_of_sizes[i];
        base_offset *= attr->child_type_extent;

        uintptr_t offset = buf_offset + base_offset * attr->u.subarray.array_of_starts[dims_offset];
        for (intptr_t i = 0; i < attr->u.subarray.array_of_subsizes[dims_offset]; i++) {
            rc = init_verify_subarray(dtp, obj, buf, attr, ndims - 1, offset, val_start,
                                      val_stride, rem_val_count, verify);
            DTPI_ERR_CHK_RC(rc);
            offset += base_offset;
        }
    } else {
        uintptr_t base_offset = 1;
        for (int i = 0; i < ndims - 1; i++)
            base_offset *= attr->u.subarray.array_of_sizes[i];
        base_offset *= attr->child_type_extent;

        uintptr_t offset = buf_offset + base_offset * attr->u.subarray.array_of_starts[ndims - 1];
        for (intptr_t i = 0; i < attr->u.subarray.array_of_subsizes[ndims - 1]; i++) {
            rc = init_verify_subarray(dtp, obj, buf, attr, ndims - 1, offset, val_start,
                                      val_stride, rem_val_count, verify);
            DTPI_ERR_CHK_RC(rc);
            offset += base_offset;
        }
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}

int DTPI_init_verify(DTP_pool_s dtp, DTP_obj_s obj, void *buf, DTPI_Attr_s * attr_tree,
                     uintptr_t buf_offset, int *val_start, int val_stride, int *rem_val_count,
                     int verify)
{
    int rc = DTP_SUCCESS;

    DTPI_FUNC_ENTER();

    if (*rem_val_count == 0)
        goto fn_exit;

    if (attr_tree == NULL) {
        rc = init_verify_base_type(dtp, obj, buf, buf_offset, val_start, val_stride, verify);
        DTPI_ERR_CHK_RC(rc);
        (*rem_val_count)--;
        goto fn_exit;
    }

    DTPI_Attr_s *attr = attr_tree;

    /* attr_idx is not at the end of the array */
    switch (attr->kind) {
        case DTPI_DATATYPE_KIND__CONTIG:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.contig.blklen; i++) {
                    rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                          val_stride, rem_val_count, verify);
                    DTPI_ERR_CHK_RC(rc);
                    offset += attr->child_type_extent;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__DUP:
            {
                rc = DTPI_init_verify(dtp, obj, buf, attr->child, buf_offset, val_start,
                                      val_stride, rem_val_count, verify);
                DTPI_ERR_CHK_RC(rc);
                break;
            }
        case DTPI_DATATYPE_KIND__RESIZED:
            {
                rc = DTPI_init_verify(dtp, obj, buf, attr->child, buf_offset, val_start,
                                      val_stride, rem_val_count, verify);
                DTPI_ERR_CHK_RC(rc);
                break;
            }
        case DTPI_DATATYPE_KIND__VECTOR:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.vector.numblks; i++) {
                    uintptr_t base_offset = offset;
                    for (int j = 0; j < attr->u.vector.blklen; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset + attr->u.vector.stride * attr->child_type_extent;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__HVECTOR:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.hvector.numblks; i++) {
                    uintptr_t base_offset = offset;
                    for (int j = 0; j < attr->u.hvector.blklen; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset + attr->u.hvector.stride;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__BLKINDX:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.blkindx.numblks; i++) {
                    uintptr_t base_offset = offset;
                    offset += attr->u.blkindx.array_of_displs[i] * attr->child_type_extent;
                    for (int j = 0; j < attr->u.blkindx.blklen; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__BLKHINDX:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.blkhindx.numblks; i++) {
                    uintptr_t base_offset = offset;
                    offset += attr->u.blkhindx.array_of_displs[i];
                    for (int j = 0; j < attr->u.blkhindx.blklen; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__INDEXED:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.indexed.numblks; i++) {
                    uintptr_t base_offset = offset;
                    offset += attr->u.indexed.array_of_displs[i] * attr->child_type_extent;
                    for (int j = 0; j < attr->u.indexed.array_of_blklens[i]; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__HINDEXED:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.hindexed.numblks; i++) {
                    uintptr_t base_offset = offset;
                    offset += attr->u.hindexed.array_of_displs[i];
                    for (int j = 0; j < attr->u.hindexed.array_of_blklens[i]; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset;
                }
                break;
            }
        case DTPI_DATATYPE_KIND__SUBARRAY:
            rc = init_verify_subarray(dtp, obj, buf, attr,
                                      attr->u.subarray.ndims, buf_offset,
                                      val_start, val_stride, rem_val_count, verify);
            DTPI_ERR_CHK_RC(rc);
            break;
        case DTPI_DATATYPE_KIND__STRUCT:
            {
                uintptr_t offset = buf_offset;
                for (intptr_t i = 0; i < attr->u.structure.numblks; i++) {
                    uintptr_t base_offset = offset;
                    offset += attr->u.structure.array_of_displs[i];
                    for (int j = 0; j < attr->u.structure.array_of_blklens[i]; j++) {
                        rc = DTPI_init_verify(dtp, obj, buf, attr->child, offset, val_start,
                                              val_stride, rem_val_count, verify);
                        DTPI_ERR_CHK_RC(rc);
                        offset += attr->child_type_extent;
                    }
                    offset = base_offset;
                }
                break;
            }
        default:
            DTPI_ERR_ASSERT(0, rc);
    }

  fn_exit:
    DTPI_FUNC_EXIT();
    return rc;

  fn_fail:
    goto fn_exit;
}
