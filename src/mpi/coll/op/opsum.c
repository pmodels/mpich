/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"

/*
 * In MPI-2.1, this operation is valid only for C integer, Fortran integer,
 * Floating point, and Complex types (5.9.2 Predefined reduce operations)
 */
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


int MPIR_SUM_check_dtype(MPI_Datatype type)
{
    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(type)) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) case (mpi_type_):
            MPIR_OP_TYPE_GROUP(INTEGER)
                MPIR_OP_TYPE_GROUP(FLOATING_POINT)
                MPIR_OP_TYPE_GROUP(C_COMPLEX)
                MPIR_OP_TYPE_GROUP(COMPLEX)
#undef MPIR_OP_TYPE_MACRO
        case MPIR_BFLOAT16:
#ifndef MPIR_FLOAT16_CTYPE
        case MPIR_FLOAT16:
#endif
            return MPI_SUCCESS;
            /* --BEGIN ERROR HANDLING-- */
        default:
            return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                        MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_SUM");
            /* --END ERROR HANDLING-- */
    }
}

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
