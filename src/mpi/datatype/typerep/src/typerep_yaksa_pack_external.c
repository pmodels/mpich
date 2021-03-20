/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"
#include <assert.h>

/* Define fixed-width equivalents for floating point types */
#if SIZEOF_FLOAT == 1
#define typerep_float8_t float
#elif SIZEOF_DOUBLE == 1
#define typerep_float8_t double
#elif SIZEOF_LONG_DOUBLE == 1
#define typerep_float8_t long double
#else
#define typerep_float8_t float
#endif

#if SIZEOF_FLOAT == 2
#define typerep_float16_t float
#elif SIZEOF_DOUBLE == 2
#define typerep_float16_t double
#elif SIZEOF_LONG_DOUBLE == 2
#define typerep_float16_t long double
#else
#define typerep_float16_t float
#endif

#if SIZEOF_FLOAT == 4
#define typerep_float32_t float
#elif SIZEOF_DOUBLE == 4
#define typerep_float32_t double
#elif SIZEOF_LONG_DOUBLE == 4
#define typerep_float32_t long double
#else
#define typerep_float32_t float
#endif

#if SIZEOF_FLOAT == 8
#define typerep_float64_t float
#elif SIZEOF_DOUBLE == 8
#define typerep_float64_t double
#elif SIZEOF_LONG_DOUBLE == 8
#define typerep_float64_t long double
#else
#define typerep_float64_t float
#endif

#if SIZEOF_FLOAT == 16
#define typerep_float128_t float
#elif SIZEOF_DOUBLE == 16
#define typerep_float128_t double
#elif SIZEOF_LONG_DOUBLE == 16
#define typerep_float128_t long double
#else
#define typerep_float128_t float
#endif

#if SIZEOF_FLOAT == 32
#define typerep_float256_t float
#elif SIZEOF_DOUBLE == 32
#define typerep_float256_t double
#elif SIZEOF_LONG_DOUBLE == 32
#define typerep_float256_t long double
#else
#define typerep_float256_t float
#endif

#define PACK_EXTERNAL(iov, outbuf, max_iov_len, src_c_type, dest_c_type) \
    do {                                                                \
        dest_c_type *dbuf = (dest_c_type *) outbuf;                     \
        uintptr_t idx = 0;                                              \
        for (uintptr_t i = 0; i < max_iov_len; i++) {                   \
            src_c_type *sbuf = (src_c_type *) iov[i].iov_base;          \
            for (size_t j = 0; j < iov[i].iov_len / sizeof(src_c_type); j++) { \
                dbuf[idx++] = (dest_c_type) sbuf[j];                    \
            }                                                           \
        }                                                               \
    } while (0)

#define PACK_EXTERNAL_INT_MAPPED(inbuf, outbuf, max_iov_len, basic_type_size, dest_c_type) \
    do {                                                                \
        switch (basic_type_size) {                                      \
        case 1:                                                         \
            PACK_EXTERNAL(inbuf, outbuf, max_iov_len, int8_t, dest_c_type); \
            break;                                                      \
                                                                        \
        case 2:                                                         \
            PACK_EXTERNAL(inbuf, outbuf, max_iov_len, int16_t, dest_c_type); \
            break;                                                      \
                                                                        \
        case 4:                                                         \
            PACK_EXTERNAL(inbuf, outbuf, max_iov_len, int32_t, dest_c_type); \
            break;                                                      \
                                                                        \
        case 8:                                                         \
            PACK_EXTERNAL(inbuf, outbuf, max_iov_len, int64_t, dest_c_type); \
            break;                                                      \
                                                                        \
        default:                                                        \
            assert(0);                                                  \
        }                                                               \
    } while (0)

#define PACK_EXTERNAL_FLOAT_MAPPED(inbuf, outbuf, max_iov_len, basic_type_size, dest_c_type) \
        do {                                                            \
            switch (basic_type_size) {                                  \
            case 1:                                                     \
                PACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float8_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 2:                                                     \
                PACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float16_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 4:                                                     \
                PACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float32_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 8:                                                     \
                PACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float64_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 16:                                                    \
                PACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float128_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 32:                                                    \
                PACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float256_t, dest_c_type); \
                break;                                                  \
                                                                        \
            default:                                                    \
                assert(0);                                              \
            }                                                           \
        } while (0)

#define UNPACK_EXTERNAL(inbuf, iov, max_iov_len, src_c_type, dest_c_type) \
    do {                                                                \
        src_c_type *sbuf = (src_c_type *) inbuf;                        \
        uintptr_t idx = 0;                                              \
        for (uintptr_t i = 0; i < max_iov_len; i++) {                   \
            dest_c_type *dbuf = (dest_c_type *) iov[i].iov_base;        \
            for (size_t j = 0; j < iov[i].iov_len / sizeof(src_c_type); j++) { \
                dbuf[j] = (dest_c_type) sbuf[idx++];                    \
            }                                                           \
        }                                                               \
    } while (0)

#define UNPACK_EXTERNAL_INT_MAPPED(inbuf, outbuf, max_iov_len, basic_type_size, dest_c_type) \
    do {                                                                \
        switch (basic_type_size) {                                      \
        case 1:                                                         \
            UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, int8_t, dest_c_type); \
            break;                                                      \
                                                                        \
        case 2:                                                         \
            UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, int16_t, dest_c_type); \
            break;                                                      \
                                                                        \
        case 4:                                                         \
            UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, int32_t, dest_c_type); \
            break;                                                      \
                                                                        \
        case 8:                                                         \
            UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, int64_t, dest_c_type); \
            break;                                                      \
                                                                        \
        default:                                                        \
            assert(0);                                                  \
        }                                                               \
    } while (0)

#define UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, outbuf, max_iov_len, basic_type_size, dest_c_type) \
        do {                                                            \
            switch (basic_type_size) {                                  \
            case 1:                                                     \
                UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float8_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 2:                                                     \
                UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float16_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 4:                                                     \
                UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float32_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 8:                                                     \
                UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float64_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 16:                                                    \
                UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float128_t, dest_c_type); \
                break;                                                  \
                                                                        \
            case 32:                                                    \
                UNPACK_EXTERNAL(inbuf, outbuf, max_iov_len, typerep_float256_t, dest_c_type); \
                break;                                                  \
                                                                        \
            default:                                                    \
                assert(0);                                              \
            }                                                           \
        } while (0)

int MPIR_Typerep_pack_external(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                               void *outbuf, MPI_Aint * actual_pack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_PACK_EXTERNAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_PACK_EXTERNAL);

    int mpi_errno = MPI_SUCCESS;
    int rc;
    struct iovec *iov = NULL;

    assert(datatype != MPI_DATATYPE_NULL);

    MPIR_Datatype *typeptr;
    MPIR_Datatype_get_ptr(datatype, typeptr);

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);

    *actual_pack_bytes = typeptr->size * incount;

    /* convert type to IOV */
    uintptr_t max_iov_len;
    rc = yaksa_iov_len(incount, type, &max_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t actual_iov_len;
    iov = MPL_malloc(max_iov_len * sizeof(struct iovec), MPL_MEM_DATATYPE);
    rc = yaksa_iov(inbuf, incount, type, 0, iov, max_iov_len, &actual_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    assert(max_iov_len == actual_iov_len);
    assert(typeptr->basic_type != MPI_DATATYPE_NULL);

    MPI_Datatype basic_type;
    if (HANDLE_IS_BUILTIN(datatype)) {
        basic_type = datatype;
    } else {
        basic_type = typeptr->basic_type;
    }

    MPI_Aint basic_type_size;
    MPIR_Datatype_get_size_macro(basic_type, basic_type_size);

    /* destination sizes are fixed in external32 packing, so we always
     * map each type to a fixed type.  See table 13.2 in the MPI-3.1
     * specification (page 540). */
    /* we do not need to distinguish between signed and unsigned types
     * because their bit representations are identical for
     * pack/unpack. */
    switch (basic_type) {
        case MPI_PACKED:
        case MPI_BYTE:
        case MPI_INT8_T:
        case MPI_UINT8_T:
        case MPI_CHAR:
        case MPI_UNSIGNED_CHAR:
        case MPI_SIGNED_CHAR:
        case MPI_C_BOOL:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_CHARACTER:
        case MPI_INTEGER1:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_BOOL:
#endif /* HAVE_CXX_BINDING */
            PACK_EXTERNAL_INT_MAPPED(iov, outbuf, max_iov_len, basic_type_size, int8_t);
            break;

        case MPI_WCHAR:
        case MPI_SHORT:
        case MPI_UNSIGNED_SHORT:
        case MPI_INT16_T:
        case MPI_UINT16_T:
        case MPI_COUNT:
        case MPI_OFFSET:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_INTEGER2:
#endif /* HAVE_FORTRAN_BINDING */
            PACK_EXTERNAL_INT_MAPPED(iov, outbuf, max_iov_len, basic_type_size, int16_t);
            break;

        case MPI_INT:
        case MPI_UNSIGNED:
        case MPI_LONG:
        case MPI_UNSIGNED_LONG:
        case MPI_INT32_T:
        case MPI_UINT32_T:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_LOGICAL:
        case MPI_INTEGER:
        case MPI_INTEGER4:
#endif /* HAVE_FORTRAN_BINDING */
            PACK_EXTERNAL_INT_MAPPED(iov, outbuf, max_iov_len, basic_type_size, int32_t);
            break;

        case MPI_LONG_LONG:
        case MPI_UNSIGNED_LONG_LONG:
        case MPI_INT64_T:
        case MPI_UINT64_T:
        case MPI_AINT:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_INTEGER8:
#endif /* HAVE_FORTRAN_BINDING */
            PACK_EXTERNAL_INT_MAPPED(iov, outbuf, max_iov_len, basic_type_size, int64_t);
            break;

#ifdef HAVE_FORTRAN_BINDING
        case MPI_FLOAT:
        case MPI_REAL:
        case MPI_REAL4:
            PACK_EXTERNAL_FLOAT_MAPPED(iov, outbuf, max_iov_len, basic_type_size,
                                       typerep_float32_t);
            break;
#endif /* HAVE_FORTRAN_BINDING */

        case MPI_DOUBLE:
        case MPI_C_COMPLEX:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_DOUBLE_PRECISION:
        case MPI_COMPLEX:
        case MPI_REAL8:
        case MPI_COMPLEX8:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_FLOAT_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            PACK_EXTERNAL_FLOAT_MAPPED(iov, outbuf, max_iov_len, basic_type_size,
                                       typerep_float64_t);
            break;

        case MPI_C_DOUBLE_COMPLEX:
        case MPI_LONG_DOUBLE:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_DOUBLE_COMPLEX:
        case MPI_COMPLEX16:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_DOUBLE_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            PACK_EXTERNAL_FLOAT_MAPPED(iov, outbuf, max_iov_len, basic_type_size,
                                       typerep_float128_t);
            break;

        case MPI_C_LONG_DOUBLE_COMPLEX:
#ifdef HAVE_FORTRAN_BINDING
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_LONG_DOUBLE_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            PACK_EXTERNAL_FLOAT_MAPPED(iov, outbuf, max_iov_len, basic_type_size,
                                       typerep_float256_t);
            break;

        default:
            /* some types are handled with if-else branches, instead
             * of a switch statement because MPICH might define them
             * as "MPI_DATATYPE_NULL". */
            if (datatype == MPI_REAL16) {
                PACK_EXTERNAL_FLOAT_MAPPED(iov, outbuf, max_iov_len, basic_type_size,
                                           typerep_float128_t);
            } else if (datatype == MPI_COMPLEX32) {
                PACK_EXTERNAL_FLOAT_MAPPED(iov, outbuf, max_iov_len, basic_type_size,
                                           typerep_float256_t);
            } else {
                /* unsupported, including MPI_INTEGER16 */
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**packextunsupport");
                goto fn_fail;
            }
    }

  fn_exit:
    if (iov)
        MPL_free(iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_PACK_EXTERNAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_unpack_external(const void *inbuf, void *outbuf, MPI_Aint outcount,
                                 MPI_Datatype datatype, MPI_Aint * actual_unpack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_UNPACK_EXTERNAL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_UNPACK_EXTERNAL);

    int mpi_errno = MPI_SUCCESS;
    int rc;
    struct iovec *iov = NULL;

    assert(datatype != MPI_DATATYPE_NULL);

    MPIR_Datatype *typeptr;
    MPIR_Datatype_get_ptr(datatype, typeptr);

    *actual_unpack_bytes = typeptr->size * outcount;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);

    /* convert type to IOV */
    uintptr_t max_iov_len;
    rc = yaksa_iov_len(outcount, type, &max_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t actual_iov_len;
    iov = MPL_malloc(max_iov_len * sizeof(struct iovec), MPL_MEM_DATATYPE);
    rc = yaksa_iov(outbuf, outcount, type, 0, iov, max_iov_len, &actual_iov_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    assert(max_iov_len == actual_iov_len);
    assert(typeptr->basic_type != MPI_DATATYPE_NULL);

    MPI_Aint basic_type;
    if (HANDLE_IS_BUILTIN(datatype)) {
        basic_type = datatype;
    } else {
        basic_type = typeptr->basic_type;
    }

    MPI_Aint basic_type_size;
    MPIR_Datatype_get_size_macro(basic_type, basic_type_size);

    /* destination sizes are fixed in external32 packing, so we always
     * map each type to a fixed type.  See table 13.2 in the MPI-3.1
     * specification (page 540). */
    /* we do not need to distinguish between signed and unsigned types
     * because their bit representations are identical for
     * pack/unpack. */
    switch (basic_type) {
        case MPI_PACKED:
        case MPI_BYTE:
        case MPI_INT8_T:
        case MPI_UINT8_T:
        case MPI_CHAR:
        case MPI_UNSIGNED_CHAR:
        case MPI_SIGNED_CHAR:
        case MPI_C_BOOL:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_CHARACTER:
        case MPI_INTEGER1:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_BOOL:
#endif /* HAVE_CXX_BINDING */
            UNPACK_EXTERNAL_INT_MAPPED(inbuf, iov, max_iov_len, basic_type_size, int8_t);
            break;

        case MPI_WCHAR:
        case MPI_SHORT:
        case MPI_UNSIGNED_SHORT:
        case MPI_INT16_T:
        case MPI_UINT16_T:
        case MPI_COUNT:
        case MPI_OFFSET:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_INTEGER2:
#endif /* HAVE_FORTRAN_BINDING */
            UNPACK_EXTERNAL_INT_MAPPED(inbuf, iov, max_iov_len, basic_type_size, int16_t);
            break;

        case MPI_INT:
        case MPI_UNSIGNED:
        case MPI_LONG:
        case MPI_UNSIGNED_LONG:
        case MPI_INT32_T:
        case MPI_UINT32_T:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_LOGICAL:
        case MPI_INTEGER:
        case MPI_INTEGER4:
#endif /* HAVE_FORTRAN_BINDING */
            UNPACK_EXTERNAL_INT_MAPPED(inbuf, iov, max_iov_len, basic_type_size, int32_t);
            break;

        case MPI_LONG_LONG:
        case MPI_UNSIGNED_LONG_LONG:
        case MPI_INT64_T:
        case MPI_UINT64_T:
        case MPI_AINT:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_INTEGER8:
#endif /* HAVE_FORTRAN_BINDING */
            UNPACK_EXTERNAL_INT_MAPPED(inbuf, iov, max_iov_len, basic_type_size, int64_t);
            break;

#ifdef HAVE_FORTRAN_BINDING
        case MPI_FLOAT:
        case MPI_REAL:
        case MPI_REAL4:
            UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, iov, max_iov_len, basic_type_size,
                                         typerep_float32_t);
            break;
#endif /* HAVE_FORTRAN_BINDING */

        case MPI_DOUBLE:
        case MPI_C_COMPLEX:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_DOUBLE_PRECISION:
        case MPI_COMPLEX:
        case MPI_REAL8:
        case MPI_COMPLEX8:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_FLOAT_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, iov, max_iov_len, basic_type_size,
                                         typerep_float64_t);
            break;

        case MPI_C_DOUBLE_COMPLEX:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_DOUBLE_COMPLEX:
        case MPI_LONG_DOUBLE:
        case MPI_COMPLEX16:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_DOUBLE_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, iov, max_iov_len, basic_type_size,
                                         typerep_float128_t);
            break;

        case MPI_C_LONG_DOUBLE_COMPLEX:
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_LONG_DOUBLE_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, iov, max_iov_len, basic_type_size,
                                         typerep_float256_t);
            break;

#ifdef HAVE_FORTRAN_BINDING
        case MPI_INTEGER16:
#endif /* HAVE_FORTRAN_BINDING */
        default:
            /* some types are handled with if-else branches, instead
             * of a switch statement because MPICH might define them
             * as "MPI_DATATYPE_NULL". */
            if (datatype == MPI_REAL16) {
                UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, iov, max_iov_len, basic_type_size,
                                             typerep_float128_t);
            } else if (datatype == MPI_COMPLEX32) {
                UNPACK_EXTERNAL_FLOAT_MAPPED(inbuf, iov, max_iov_len, basic_type_size,
                                             typerep_float256_t);
            } else {
                /* unsupported, including MPI_INTEGER16 */
                MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**packextunsupport");
                goto fn_fail;
            }
    }

  fn_exit:
    if (iov)
        MPL_free(iov);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_UNPACK_EXTERNAL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_size_external32(MPI_Datatype type)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_SIZE_EXTERNAL32);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_SIZE_EXTERNAL32);

    int size;

    assert(type != MPI_DATATYPE_NULL);

    MPIR_Datatype *typeptr;
    MPIR_Datatype_get_ptr(type, typeptr);

    MPI_Aint basic_type;
    if (HANDLE_IS_BUILTIN(type)) {
        basic_type = type;
    } else {
        basic_type = typeptr->basic_type;
    }

    MPI_Aint basic_type_size;
    MPIR_Datatype_get_size_macro(basic_type, basic_type_size);

    size = typeptr->size / basic_type_size;

    switch (basic_type) {
        case MPI_PACKED:
        case MPI_BYTE:
        case MPI_INT8_T:
        case MPI_UINT8_T:
        case MPI_CHAR:
        case MPI_UNSIGNED_CHAR:
        case MPI_SIGNED_CHAR:
        case MPI_C_BOOL:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_CHARACTER:
        case MPI_INTEGER1:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_BOOL:
#endif /* HAVE_CXX_BINDING */
            size *= 1;
            break;

        case MPI_WCHAR:
        case MPI_SHORT:
        case MPI_UNSIGNED_SHORT:
        case MPI_INT16_T:
        case MPI_UINT16_T:
        case MPI_COUNT:
        case MPI_OFFSET:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_INTEGER2:
#endif /* HAVE_FORTRAN_BINDING */
            size *= 2;
            break;

        case MPI_INT:
        case MPI_UNSIGNED:
        case MPI_LONG:
        case MPI_UNSIGNED_LONG:
        case MPI_INT32_T:
        case MPI_UINT32_T:
        case MPI_FLOAT:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_LOGICAL:
        case MPI_INTEGER:
        case MPI_INTEGER4:
        case MPI_REAL:
        case MPI_REAL4:
#endif /* HAVE_FORTRAN_BINDING */
            size *= 4;
            break;

        case MPI_LONG_LONG:
        case MPI_UNSIGNED_LONG_LONG:
        case MPI_INT64_T:
        case MPI_UINT64_T:
        case MPI_AINT:
        case MPI_INTEGER8:
        case MPI_DOUBLE:
        case MPI_C_COMPLEX:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_DOUBLE_PRECISION:
        case MPI_COMPLEX:
        case MPI_REAL8:
        case MPI_COMPLEX8:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_FLOAT_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            size *= 8;
            break;

        case MPI_C_DOUBLE_COMPLEX:
        case MPI_LONG_DOUBLE:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_DOUBLE_COMPLEX:
        case MPI_COMPLEX16:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_DOUBLE_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            size *= 16;
            break;

        case MPI_C_LONG_DOUBLE_COMPLEX:
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_LONG_DOUBLE_COMPLEX:
#endif /* HAVE_CXX_BINDING */
            size *= 32;
            break;

        default:
            /* some types are handled with if-else branches, instead
             * of a switch statement because MPICH might define them
             * as "MPI_DATATYPE_NULL". */
            if (type == MPI_REAL16)
                size *= 16;
            else if (type == MPI_COMPLEX32)
                size *= 32;
            else if (type == MPI_INTEGER16)
                size *= 16;
            else
                assert(0);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_SIZE_EXTERNAL32);
    return size;
}
