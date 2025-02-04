/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"
#include "mpii_fortlogical.h"

/* -- MPI_SUM -- */

#define MPIR_LSUM(a,b) ((a)+(b))

static void bfloat16_sum(void *invec, void *inoutvec, MPI_Aint len);
static void f16_sum(void *invec, void *inoutvec, MPI_Aint len);

void MPIR_SUM(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LSUM)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                MPIR_OP_TYPE_GROUP(C_COMPLEX)
                /* complex addition is slightly different than scalar addition */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): {                                \
            c_type_ * restrict a = (c_type_ *)inoutvec;    \
            const c_type_ * restrict b = (c_type_ *)invec; \
            for (i=0; i<len; i++) {                      \
                a[i].re = MPIR_LSUM(a[i].re ,b[i].re);     \
                a[i].im = MPIR_LSUM(a[i].im ,b[i].im);     \
            }                                              \
            break;                                         \
        }
                MPIR_OP_TYPE_GROUP(COMPLEX)
        case MPIR_BFLOAT16:
            bfloat16_sum(invec, inoutvec, len);
            break;
#ifndef MPIR_FLOAT16_CTYPE
        case MPIR_FLOAT16:
            f16_sum(invec, inoutvec, len);
            break;
#endif
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_PROD -- */

#define MPIR_LPROD(a,b) ((a)*(b))

void MPIR_PROD(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LPROD)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                MPIR_OP_TYPE_GROUP(C_COMPLEX)
                /* complex multiplication is slightly different than scalar multiplication */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): {                             \
            c_type_ * restrict a = (c_type_ *)inoutvec; \
            c_type_ * restrict b = (c_type_ *)invec;    \
            for (i=0; i<len; i++) {                   \
                c_type_ c;                              \
                c.re = a[i].re; c.im = a[i].im;         \
                a[i].re = c.re*b[i].re - c.im*b[i].im;  \
                a[i].im = c.im*b[i].re + c.re*b[i].im;  \
            }                                           \
            break;                                      \
        }
                MPIR_OP_TYPE_GROUP(COMPLEX)
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_LAND -- */

#ifndef MPIR_LLAND
#define MPIR_LLAND(a,b) ((a)&&(b))
#endif

void MPIR_LAND(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LLAND)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): { \
            c_type_ * restrict a = (c_type_ *)inoutvec; \
            c_type_ * restrict b = (c_type_ *)invec; \
            for (i=0; i<len; i++) { \
                a[i] = MPII_TO_FLOG(MPIR_LLAND(MPII_FROM_FLOG(a[i]), MPII_FROM_FLOG(b[i]))); \
            } \
            break; \
        }
                MPIR_OP_TYPE_GROUP(FORTRAN_LOGICAL)
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_LOR -- */

#ifndef MPIR_LLOR
#define MPIR_LLOR(a,b) ((a)||(b))
#endif

void MPIR_LOR(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LLOR)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): { \
            c_type_ * restrict a = (c_type_ *)inoutvec; \
            c_type_ * restrict b = (c_type_ *)invec; \
            for (i=0; i<len; i++) { \
                a[i] = MPII_TO_FLOG(MPIR_LLOR(MPII_FROM_FLOG(a[i]), MPII_FROM_FLOG(b[i]))); \
            } \
            break; \
        }
                MPIR_OP_TYPE_GROUP(FORTRAN_LOGICAL)
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_LXOR -- */

#ifndef MPIR_LLXOR
#define MPIR_LLXOR(a,b) (((a)&&(!b))||((!a)&&(b)))
#endif

void MPIR_LXOR(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LLXOR)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
        case (mpi_type_): { \
            c_type_ * restrict a = (c_type_ *)inoutvec; \
            c_type_ * restrict b = (c_type_ *)invec; \
            for (i=0; i<len; i++) { \
                a[i] = MPII_TO_FLOG(MPIR_LLXOR(MPII_FROM_FLOG(a[i]), MPII_FROM_FLOG(b[i]))); \
            } \
            break; \
        }
                MPIR_OP_TYPE_GROUP(FORTRAN_LOGICAL)
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_BAND -- */

#ifndef MPIR_LBAND
#define MPIR_LBAND(a,b) ((a)&(b))
#endif

void MPIR_BAND(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LBAND)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_LBOR -- */

#ifndef MPIR_LBOR
#define MPIR_LBOR(a,b) ((a)|(b))
#endif

void MPIR_BOR(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LBOR)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_BXOR -- */

#ifndef MPIR_LBXOR
#define MPIR_LBXOR(a,b) ((a)^(b))
#endif

void MPIR_BXOR(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPIR_LBXOR)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
#undef MPIR_OP_TYPE_MACRO
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_MAX -- */

void MPIR_MAXF(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPL_MAX)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_MIN -- */

void MPIR_MINF(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPI_Aint i, len = *Len;

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(*type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_, c_type_, MPL_MIN)
            /* no semicolons by necessity */
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
        default:
            MPIR_Assert(0);
            break;
    }
}

/* -- MPI_MAXLOC -- */

/* Note a subtlety in these two macros which avoids compiler warnings.
   The compiler complains about using == on floats, but the standard
   requires that we set loc to min of the locs if the two values are
   equal.  So we do "if a<b {} else if a<=b Y" which is the same as
   "if a<b X else if a==b Y" but avoids the warning. */

void MPIR_MAXLOC(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPIR_Assert(MPIR_Datatype_is_pairtype(*type));

    if (HANDLE_IS_BUILTIN(*type) && (*type & MPIR_TYPE_PAIR_MASK)) {
        MPI_Aint len2 = (*Len) * 2;
        /* bits mask magic: same MPIR_TYPE without PAIR_MASK, and cut the type size by half */
        MPI_Datatype value_type = 0x4c000000 | (*type & 0x8f0000) | ((*type & 0xff00) >> 1);
        switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
            case (mpi_type_) : { \
                c_type_ * restrict a = inoutvec; \
                const c_type_ * restrict b = invec; \
                for (MPI_Aint i=0; i<len2; i+=2) { \
                    if (a[i] < b[i]) { \
                        a[i]   = b[i]; \
                        a[i+1] = b[i+1]; \
                    } else if (a[i] <= b[i]) { \
                        a[i+1] = MPL_MIN(a[i+1],b[i+1]); \
                    } \
                } \
                break; \
            }

                /* no semicolons by necessity */
                MPIR_OP_TYPE_GROUP(INTEGER)
                    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
            default:
                MPIR_Assert(0);
                break;
        }
    } else {
        MPI_Datatype value_type = MPIR_Pairtype_get_value_type(*type);
        switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
            case (mpi_type_) : { \
                struct _pair { \
                    c_type_ value; \
                    int loc; \
                }; \
                struct _pair * restrict a = inoutvec; \
                const struct _pair * restrict b = invec; \
                for (MPI_Aint i=0; i<*Len; i++) { \
                    if (a[i].value < b[i].value) { \
                        a[i].value = b[i].value; \
                        a[i].loc   = b[i].loc; \
                    } else if (a[i].value <= b[i].value) { \
                        a[i].loc = MPL_MIN(a[i].loc,b[i].loc); \
                    } \
                } \
                break; \
            }

                /* no semicolons by necessity */
                MPIR_OP_TYPE_GROUP(INTEGER)
                    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
            default:
                MPIR_Assert(0);
                break;
        }
    }
}

/* -- MPI_MINLOC -- */

/* Note a subtlety in these two macros which avoids compiler warnings.
   The compiler complains about using == on floats, but the standard
   requires that we set loc to min of the locs if the two values are
   equal.  So we do "if a>b {} else if a>=b Y" which is the same as
   "if a>b X else if a==b Y" but avoids the warning. */

void MPIR_MINLOC(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPIR_Assert(MPIR_Datatype_is_pairtype(*type));

    if (HANDLE_IS_BUILTIN(*type) && (*type & MPIR_TYPE_PAIR_MASK)) {
        MPI_Aint len2 = (*Len) * 2;
        /* bits mask magic: same MPIR_TYPE without PAIR_MASK, and cut the type size by half */
        MPI_Datatype value_type = 0x4c000000 | (*type & 0x8f0000) | ((*type & 0xff00) >> 1);
        switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
            case (mpi_type_) : { \
                c_type_ * restrict a = inoutvec; \
                const c_type_ * restrict b = invec; \
                for (MPI_Aint i=0; i<len2; i+=2) { \
                    if (a[i] > b[i]) { \
                        a[i]   = b[i]; \
                        a[i+1] = b[i+1]; \
                    } else if (a[i] >= b[i]) { \
                        a[i+1] = MPL_MIN(a[i+1],b[i+1]); \
                    } \
                } \
                break; \
            }

                /* no semicolons by necessity */
                MPIR_OP_TYPE_GROUP(INTEGER)
                    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
            default:
                MPIR_Assert(0);
                break;
        }
    } else {
        MPI_Datatype value_type = MPIR_Pairtype_get_value_type(*type);
        switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
            case (mpi_type_) : { \
                struct _pair { \
                    c_type_ value; \
                    int loc; \
                }; \
                struct _pair * restrict a = inoutvec; \
                const struct _pair * restrict b = invec; \
                for (MPI_Aint i=0; i<*Len; i++) { \
                    if (a[i].value > b[i].value) { \
                        a[i].value = b[i].value; \
                        a[i].loc   = b[i].loc; \
                    } else if (a[i].value >= b[i].value) { \
                        a[i].loc = MPL_MIN(a[i].loc,b[i].loc); \
                    } \
                } \
                break; \
            }

                /* no semicolons by necessity */
                MPIR_OP_TYPE_GROUP(INTEGER)
                    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
            default:
                MPIR_Assert(0);
                break;
        }
    }
}

/* -- MPI_NO_OP -- */

void MPIR_NO_OP(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    return;
}

/* -- MPI_REPLACE -- */

void MPIR_REPLACE(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Localcopy(invec, *Len, *type, inoutvec, *Len, *type);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return;
  fn_fail:
    goto fn_exit;
}

/* ---- internal routines ------------------------- */

/* BFloat16 - software arithemetics
 * TODO: add hardware support, e.g. via AVX512 intrinsics
 */
static float bfloat16_load(void *p)
{
    uint32_t u = ((uint32_t) (*(uint16_t *) p) << 16);
    float v;
    memcpy(&v, &u, sizeof(float));
    return v;
}

static void bfloat16_store(void *p, float v)
{
    uint32_t u;
    memcpy(&u, &v, sizeof(float));
    if (u & 0x8000) {
        /* round up */
        *(uint16_t *) p = (u >> 16) + 1;
    } else {
        /* truncation */
        *(uint16_t *) p = (u >> 16);
    }

}

static void bfloat16_sum(void *invec, void *inoutvec, MPI_Aint len)
{
    for (MPI_Aint i = 0; i < len * 2; i += 2) {
        float a = bfloat16_load((char *) inoutvec + i);
        float b = bfloat16_load((char *) invec + i);
        bfloat16_store((char *) inoutvec + i, a + b);
    }
}

/* IEEE half-precision 16-bit float - software arithemetics
 */
static float f16_load(void *p)
{
    uint16_t a = *(uint16_t *) p;
    /* expand exponent from 5 bit to 8 bit, fraction from 10 bit to 23 bit */
    uint32_t u = ((uint32_t) ((a & 0x8000) | ((((a & 0x3c00) >> 10) + 0x70) << 7)) << 16) |
        ((uint32_t) (a & 0x3ff) << 13);
    float v;
    memcpy(&v, &u, sizeof(float));
    return v;
}

static void f16_store(void *p, float v)
{
    uint32_t u;
    memcpy(&u, &v, sizeof(float));
    /* shrink exponent from 8 bit to 5 bit, fraction from 23 bit to 10 bit */
    uint16_t a = ((u & 0x80000000) >> 16) | ((((u & 0x7f800000) >> 23) - 0x70) << 10) |
        ((u & 0x7fffff) >> 16);
    if (u & 0x1000) {
        /* round up */
        a += 1;
    }
    *(uint16_t *) p = a;
}

static void f16_sum(void *invec, void *inoutvec, MPI_Aint len)
{
    for (MPI_Aint i = 0; i < len * 2; i += 2) {
        float a = f16_load((char *) inoutvec + i);
        float b = f16_load((char *) invec + i);
        f16_store((char *) inoutvec + i, a + b);
    }
}
