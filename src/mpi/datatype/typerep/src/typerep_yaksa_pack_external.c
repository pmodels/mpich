/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"
#include "typerep_util.h"
#include <assert.h>

#define PACK_EXTERNAL_equal_size(iov, outbuf, max_iov_len, c_type) \
    do {                                                                \
        c_type *dbuf = (c_type *) outbuf;                               \
        uintptr_t idx = 0;                                              \
        for (uintptr_t i = 0; i < max_iov_len; i++) {                   \
            c_type *sbuf = (c_type *) iov[i].iov_base;                  \
            for (size_t j = 0; j < iov[i].iov_len / sizeof(c_type); j++) { \
                BASIC_convert(sbuf[j], dbuf[idx]);                      \
                idx++;                                                  \
            }                                                           \
        }                                                               \
    } while (0)

#define UNPACK_EXTERNAL_equal_size(inbuf, iov, max_iov_len, c_type) \
    do {                                                                \
        const c_type *sbuf = inbuf;                                     \
        uintptr_t idx = 0;                                              \
        for (uintptr_t i = 0; i < max_iov_len; i++) {                   \
            c_type *dbuf = (c_type *) iov[i].iov_base;                  \
            for (size_t j = 0; j < iov[i].iov_len / sizeof(c_type); j++) { \
                BASIC_convert(sbuf[idx], dbuf[j]);                      \
                idx++;                                                  \
            }                                                           \
        }                                                               \
    } while (0)

#define PACK_EXTERNAL_unequal_size(iov, outbuf, max_iov_len, c_type, pack_c_type) \
    do {                                                                \
        pack_c_type tmp;                                                \
        pack_c_type *dbuf = outbuf;                                     \
        uintptr_t idx = 0;                                              \
        for (uintptr_t i = 0; i < max_iov_len; i++) {                   \
            c_type *sbuf = (c_type *) iov[i].iov_base;                  \
            for (size_t j = 0; j < iov[i].iov_len / sizeof(c_type); j++) { \
                tmp = sbuf[j];                                          \
                BASIC_convert(tmp, dbuf[idx]);                          \
                idx++;                                                  \
            }                                                           \
        }                                                               \
    } while (0)

#define UNPACK_EXTERNAL_unequal_size(inbuf, iov, max_iov_len, c_type, pack_c_type) \
    do {                                                                \
        pack_c_type tmp;                                                \
        const pack_c_type *sbuf = inbuf;                                \
        uintptr_t idx = 0;                                              \
        for (uintptr_t i = 0; i < max_iov_len; i++) {                   \
            c_type *dbuf = (c_type *) iov[i].iov_base;                  \
            for (size_t j = 0; j < iov[i].iov_len / sizeof(c_type); j++) { \
                BASIC_convert(sbuf[idx], tmp);                          \
                dbuf[j] = tmp;                                          \
                idx++;                                                  \
            }                                                           \
        }                                                               \
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

    MPI_Aint pack_size = MPIR_Typerep_size_external32(datatype);
    *actual_pack_bytes = pack_size * incount;

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
    /* FIXME: assumes a single basic_type, won't work with struct */
    if (HANDLE_IS_BUILTIN(datatype)) {
        basic_type = datatype;
    } else {
        basic_type = typeptr->basic_type;
    }

    MPI_Aint basic_type_size, ext_type_size;
    MPIR_Datatype_get_size_macro(basic_type, basic_type_size);
    ext_type_size = MPII_Typerep_get_basic_size_external32(basic_type);

    if (MPII_Typerep_basic_type_is_complex(basic_type)) {
        /* treat as float */
        basic_type_size /= 2;
        ext_type_size /= 2;
    }

    if (basic_type_size == ext_type_size) {
        if (basic_type_size == 1) {
            PACK_EXTERNAL_equal_size(iov, outbuf, max_iov_len, int8_t);
        } else if (basic_type_size == 2) {
            PACK_EXTERNAL_equal_size(iov, outbuf, max_iov_len, int16_t);
        } else if (basic_type_size == 4) {
            PACK_EXTERNAL_equal_size(iov, outbuf, max_iov_len, int32_t);
        } else if (basic_type_size == 8) {
            PACK_EXTERNAL_equal_size(iov, outbuf, max_iov_len, int64_t);
        } else {
            MPIR_Assert(0);
        }
    } else if (basic_type == MPI_LONG_DOUBLE) {
        /* FIXME */
        MPIR_Assert(0);
    } else {
        if (basic_type_size == 1) {
            MPIR_Assert(0);
        } else if (basic_type_size == 2) {
            MPIR_Assert(0);
        } else if (basic_type_size == 4) {
            MPIR_Assert(0);
        } else if (basic_type_size == 8) {
            if (ext_type_size == 4) {
                if (MPII_Typerep_basic_type_is_unsigned(basic_type)) {
                    PACK_EXTERNAL_unequal_size(iov, outbuf, max_iov_len, uint64_t, uint32_t);
                } else {
                    PACK_EXTERNAL_unequal_size(iov, outbuf, max_iov_len, int64_t, int32_t);
                }
            } else {
                MPIR_Assert(0);
            }
        } else {
            MPIR_Assert(0);
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

    MPI_Aint pack_size = MPIR_Typerep_size_external32(datatype);
    *actual_unpack_bytes = pack_size * outcount;

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
    /* FIXME: assumes a single basic_type, won't work with struct */
    if (HANDLE_IS_BUILTIN(datatype)) {
        basic_type = datatype;
    } else {
        basic_type = typeptr->basic_type;
    }

    MPI_Aint basic_type_size, ext_type_size;
    MPIR_Datatype_get_size_macro(basic_type, basic_type_size);
    ext_type_size = MPII_Typerep_get_basic_size_external32(basic_type);

    if (MPII_Typerep_basic_type_is_complex(basic_type)) {
        /* treat as float */
        basic_type_size /= 2;
        ext_type_size /= 2;
    }

    if (basic_type_size == ext_type_size) {
        if (basic_type_size == 1) {
            UNPACK_EXTERNAL_equal_size(inbuf, iov, max_iov_len, int8_t);
        } else if (basic_type_size == 2) {
            UNPACK_EXTERNAL_equal_size(inbuf, iov, max_iov_len, int16_t);
        } else if (basic_type_size == 4) {
            UNPACK_EXTERNAL_equal_size(inbuf, iov, max_iov_len, int32_t);
        } else if (basic_type_size == 8) {
            UNPACK_EXTERNAL_equal_size(inbuf, iov, max_iov_len, int64_t);
        } else {
            MPIR_Assert(0);
        }
    } else if (basic_type == MPI_LONG_DOUBLE) {
        /* FIXME */
        MPIR_Assert(0);
    } else {
        if (basic_type_size == 1) {
            MPIR_Assert(0);
        } else if (basic_type_size == 2) {
            MPIR_Assert(0);
        } else if (basic_type_size == 4) {
            MPIR_Assert(0);
        } else if (basic_type_size == 8) {
            if (ext_type_size == 4) {
                if (MPII_Typerep_basic_type_is_unsigned(basic_type)) {
                    UNPACK_EXTERNAL_unequal_size(inbuf, iov, max_iov_len, uint64_t, uint32_t);
                } else {
                    UNPACK_EXTERNAL_unequal_size(inbuf, iov, max_iov_len, int64_t, int32_t);
                }
            } else {
                MPIR_Assert(0);
            }
        } else {
            MPIR_Assert(0);
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
    /* FIXME: assumes a single basic_type, won't work with struct */
    if (HANDLE_IS_BUILTIN(type)) {
        basic_type = type;
    } else {
        basic_type = typeptr->basic_type;
    }

    MPI_Aint basic_type_size;
    MPIR_Datatype_get_size_macro(basic_type, basic_type_size);

    size = typeptr->size / basic_type_size;
    size *= MPII_Typerep_get_basic_size_external32(basic_type);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_SIZE_EXTERNAL32);
    return size;
}
