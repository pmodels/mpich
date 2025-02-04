/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"
#include "mpii_fortlogical.h"

/* -- MPI_SUM -- */

#define MPIR_LSUM(a,b) ((a)+(b))

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
