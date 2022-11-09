/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_YAKSA_COMPLEX_SUPPORT
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR indicates that complex type reduction is not supported in yaksa.

    - name        : MPIR_CVAR_GPU_DOUBLE_SUPPORT
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR indicates that double type is not supported on the GPU.

    - name        : MPIR_CVAR_GPU_LONG_DOUBLE_SUPPORT
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR indicates that double type is not supported on the GPU.

    - name        : MPIR_CVAR_ENABLE_YAKSA_REDUCTION
      category    : COLLECTIVE
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar enables yaksa based reduction for local reduce.
=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define IS_HOST(attr) \
    ((attr).type == MPL_GPU_POINTER_UNREGISTERED_HOST || \
    (attr).type == MPL_GPU_POINTER_REGISTERED_HOST)

/* When a returned typerep_req is expected, using the nonblocking yaksa routine and
 * return the request; otherwise use the blocking yaksa routine. */
static int typerep_do_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                           MPIR_Typerep_req * typerep_req, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (typerep_req) {
        *typerep_req = MPIR_TYPEREP_REQ_NULL;
    }

    if (num_bytes == 0) {
        goto fn_exit;
    }

    MPL_pointer_attr_t inattr, outattr;
    MPIR_GPU_query_pointer_attr(inbuf, &inattr);
    MPIR_GPU_query_pointer_attr(outbuf, &outattr);

    if (IS_HOST(inattr) && IS_HOST(outattr)) {
        MPIR_Memcpy(outbuf, inbuf, num_bytes);
    } else {
        uintptr_t actual_pack_bytes;

        yaksa_info_t info = MPII_yaksa_get_info(&inattr, &outattr);
        if (typerep_req) {
            rc = yaksa_ipack(inbuf, num_bytes, YAKSA_TYPE__BYTE, 0, outbuf, num_bytes,
                             &actual_pack_bytes, info, YAKSA_OP__REPLACE,
                             (yaksa_request_t *) typerep_req);
        } else {
            rc = yaksa_pack(inbuf, num_bytes, YAKSA_TYPE__BYTE, 0, outbuf, num_bytes,
                            &actual_pack_bytes, info, YAKSA_OP__REPLACE);
        }
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
        MPIR_Assert(actual_pack_bytes == num_bytes);

        rc = MPII_yaksa_free_info(info);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* When a returned typerep_req is expected, using the nonblocking yaksa routine and
 * return the request; otherwise use the blocking yaksa routine. */
static int typerep_do_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                           MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                           MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typerep_req,
                           uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (typerep_req) {
        *typerep_req = MPIR_TYPEREP_REQ_NULL;
    }

    if (incount == 0) {
        *actual_pack_bytes = 0;
        goto fn_exit;
    }

    MPIR_Assert(datatype != MPI_DATATYPE_NULL);

    int is_contig = 0;
    int element_size = -1;
    const void *inbuf_ptr;      /* adjusted by true_lb */
    MPI_Aint total_size = 0;
    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
        element_size = MPIR_Datatype_get_basic_size(datatype);
        inbuf_ptr = inbuf;
        total_size = incount * element_size;
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(datatype, dtp);
        is_contig = dtp->is_contig;
        element_size = dtp->builtin_element_size;
        inbuf_ptr = MPIR_get_contig_ptr(inbuf, dtp->true_lb);
        total_size = incount * dtp->size;
    }

    MPL_pointer_attr_t inattr, outattr;

    /* only query the pointer attributes in case of relative addressing */
    bool rel_addressing = (inbuf != MPI_BOTTOM);
    if (rel_addressing) {
        MPIR_GPU_query_pointer_attr(inbuf_ptr, &inattr);
        MPIR_GPU_query_pointer_attr(outbuf, &outattr);
    }

    if (rel_addressing && is_contig && element_size > 0 && IS_HOST(inattr) && IS_HOST(outattr)) {
        MPI_Aint real_bytes = MPL_MIN(total_size - inoffset, max_pack_bytes);
        /* Make sure we never pack partial element */
        real_bytes -= real_bytes % element_size;
        if (flags & MPIR_TYPEREP_FLAG_STREAM) {
            MPIR_Memcpy_stream(outbuf, MPIR_get_contig_ptr(inbuf_ptr, inoffset), real_bytes);
        } else {
            MPIR_Memcpy(outbuf, MPIR_get_contig_ptr(inbuf_ptr, inoffset), real_bytes);
        }
        *actual_pack_bytes = real_bytes;
        goto fn_exit;
    }

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_info_t info = (rel_addressing) ? MPII_yaksa_get_info(&inattr, &outattr) : NULL;

    uintptr_t real_pack_bytes;
    if (typerep_req) {
        rc = yaksa_ipack(inbuf, incount, type, inoffset, outbuf, max_pack_bytes,
                         &real_pack_bytes, info, YAKSA_OP__REPLACE,
                         (yaksa_request_t *) typerep_req);
    } else {
        rc = yaksa_pack(inbuf, incount, type, inoffset, outbuf, max_pack_bytes,
                        &real_pack_bytes, info, YAKSA_OP__REPLACE);
    }
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    if (info) {
        rc = MPII_yaksa_free_info(info);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    }

    *actual_pack_bytes = (MPI_Aint) real_pack_bytes;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function checks whether the operation is supported in yaksa for the
 * provided datatype */
int MPIR_Typerep_reduce_is_supported(MPI_Op op, MPI_Datatype datatype)
{
    yaksa_op_t yaksa_op;

    if (!MPIR_CVAR_ENABLE_YAKSA_REDUCTION)
        return 0;

    /* yaksa pup code currently treat unsigned integer type the same as
     * the corresponding signed integer type, which will not work with
     * most op other than REPLACE.
     */
    if (datatype == MPI_UNSIGNED_CHAR ||
        datatype == MPI_UNSIGNED_SHORT ||
        datatype == MPI_UNSIGNED ||
        datatype == MPI_UNSIGNED_LONG ||
        datatype == MPI_UNSIGNED_LONG_LONG ||
        datatype == MPI_UINT8_T ||
        datatype == MPI_UINT16_T || datatype == MPI_UINT32_T || datatype == MPI_UINT64_T) {
        return 0;
    }

    if ((datatype == MPI_DOUBLE || datatype == MPI_C_DOUBLE_COMPLEX) &&
        (!MPIR_CVAR_GPU_DOUBLE_SUPPORT))
        return 0;
    if ((datatype == MPI_LONG_DOUBLE || datatype == MPI_C_LONG_DOUBLE_COMPLEX) &&
        (!MPIR_CVAR_GPU_LONG_DOUBLE_SUPPORT))
        return 0;

    if ((datatype == MPI_C_COMPLEX || datatype == MPI_C_DOUBLE_COMPLEX ||
         datatype == MPI_C_LONG_DOUBLE_COMPLEX) && (!MPIR_CVAR_YAKSA_COMPLEX_SUPPORT))
        return 0;
#ifdef HAVE_FORTRAN_BINDING
    if ((datatype == MPI_COMPLEX || datatype == MPI_DOUBLE_COMPLEX || datatype == MPI_COMPLEX8 ||
         datatype == MPI_COMPLEX16) && (!MPIR_CVAR_YAKSA_COMPLEX_SUPPORT))
        return 0;
    if (datatype == MPI_DOUBLE_COMPLEX && (!MPIR_CVAR_GPU_DOUBLE_SUPPORT))
        return 0;
#endif
#ifdef HAVE_CXX_BINDING
    if ((datatype == MPI_CXX_FLOAT_COMPLEX || datatype == MPI_CXX_DOUBLE_COMPLEX ||
         datatype == MPI_CXX_LONG_DOUBLE_COMPLEX) && (!MPIR_CVAR_YAKSA_COMPLEX_SUPPORT))
        return 0;
    if (datatype == MPI_CXX_DOUBLE_COMPLEX && (!MPIR_CVAR_GPU_DOUBLE_SUPPORT))
        return 0;
    if (datatype == MPI_CXX_LONG_DOUBLE_COMPLEX && (!MPIR_CVAR_GPU_LONG_DOUBLE_SUPPORT))
        return 0;
#endif
    if ((datatype == MPI_FLOAT || datatype == MPI_DOUBLE || datatype == MPI_LONG_DOUBLE) &&
        (op == MPI_LXOR || op == MPI_LAND || op == MPI_LOR))
        return 0;

    yaksa_op = MPII_Typerep_get_yaksa_op(op);

    if (yaksa_op == YAKSA_OP__LAST)
        return 0;
    return 1;
}

/* When a returned typerep_req is expected, using the nonblocking yaksa routine and
 * return the request; otherwise use the blocking yaksa routine. */
static int typerep_do_unpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                             MPI_Datatype datatype, MPI_Aint outoffset,
                             MPI_Aint * actual_unpack_bytes, MPIR_Typerep_req * typerep_req,
                             uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (typerep_req) {
        *typerep_req = MPIR_TYPEREP_REQ_NULL;
    }

    if (insize == 0) {
        *actual_unpack_bytes = 0;
        goto fn_exit;
    }

    MPIR_Assert(datatype != MPI_DATATYPE_NULL);

    int is_contig = 0;
    int element_size = -1;
    const void *outbuf_ptr;     /* adjusted by true_lb */
    MPI_Aint total_size = 0;
    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
        element_size = MPIR_Datatype_get_basic_size(datatype);
        outbuf_ptr = outbuf;
        total_size = outcount * element_size;
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(datatype, dtp);
        is_contig = dtp->is_contig;
        element_size = dtp->builtin_element_size;
        outbuf_ptr = MPIR_get_contig_ptr(outbuf, dtp->true_lb);
        total_size = outcount * dtp->size;
    }

    MPL_pointer_attr_t inattr, outattr;

    /* only query the pointer attributes in case of relative addressing */
    bool rel_addressing = (outbuf != MPI_BOTTOM);
    if (rel_addressing) {
        MPIR_GPU_query_pointer_attr(inbuf, &inattr);
        MPIR_GPU_query_pointer_attr(outbuf_ptr, &outattr);
    }

    if (rel_addressing && is_contig && IS_HOST(inattr) && IS_HOST(outattr)) {
        *actual_unpack_bytes = MPL_MIN(total_size - outoffset, insize);
        /* We assume the amount we unpack is multiple of element_size */
        MPIR_Assert(element_size < 0 || *actual_unpack_bytes % element_size == 0);
        MPIR_Memcpy(MPIR_get_contig_ptr(outbuf_ptr, outoffset), inbuf, *actual_unpack_bytes);
        goto fn_exit;
    }

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_info_t info = (rel_addressing) ? MPII_yaksa_get_info(&inattr, &outattr) : NULL;

    uintptr_t real_insize = MPL_MIN(total_size - outoffset, insize);

    uintptr_t real_unpack_bytes;
    if (typerep_req) {
        rc = yaksa_iunpack(inbuf, real_insize, outbuf, outcount, type, outoffset,
                           &real_unpack_bytes, info, YAKSA_OP__REPLACE,
                           (yaksa_request_t *) typerep_req);
    } else {
        rc = yaksa_unpack(inbuf, real_insize, outbuf, outcount, type, outoffset, &real_unpack_bytes,
                          info, YAKSA_OP__REPLACE);
    }
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    if (info) {
        rc = MPII_yaksa_free_info(info);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    }

    *actual_unpack_bytes = (MPI_Aint) real_unpack_bytes;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_icopy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                       MPIR_Typerep_req * typerep_req, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_copy(outbuf, inbuf, num_bytes, typerep_req, flags);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_copy(outbuf, inbuf, num_bytes, NULL, flags);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Typerep_ipack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                       MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                       MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typerep_req, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_pack(inbuf, incount, datatype, inoffset, outbuf, max_pack_bytes,
                                actual_pack_bytes, typerep_req, flags);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_pack(inbuf, incount, datatype, inoffset, outbuf, max_pack_bytes,
                                actual_pack_bytes, NULL, flags);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Typerep_iunpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                         MPI_Datatype datatype, MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes,
                         MPIR_Typerep_req * typerep_req, uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_unpack(inbuf, insize, outbuf, outcount, datatype, outoffset,
                                  actual_unpack_bytes, typerep_req, flags);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                        MPI_Datatype datatype, MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes,
                        uint32_t flags)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_unpack(inbuf, insize, outbuf, outcount, datatype, outoffset,
                                  actual_unpack_bytes, NULL, flags);

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIR_Typerep_wait(MPIR_Typerep_req typerep_req)
{
    MPIR_FUNC_ENTER;
    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (typerep_req == MPIR_TYPEREP_REQ_NULL)
        goto fn_exit;

    rc = yaksa_request_wait((yaksa_request_t) typerep_req);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* MPIR_Typerep_op - accumulate source_buf onto target_buf with op.
 * Assume (we may relax and extend in the future)
 *   - source datatype is predefined (basic or pairtype)
 *   - target datatype may be derived with the same basic type as source
 *   - op is builtin op
 */
static int typerep_op_unpack(void *source_buf, void *target_buf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, int mapped_device);
static int typerep_op_pack(void *source_buf, void *target_buf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Op op, int mapped_device);

int MPIR_Typerep_reduce(const void *in_buf, void *out_buf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = typerep_op_pack((void *) in_buf, out_buf, count, datatype, op, -1);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_op(void *source_buf, MPI_Aint source_count, MPI_Datatype source_dtp,
                    void *target_buf, MPI_Aint target_count, MPI_Datatype target_dtp, MPI_Op op,
                    bool source_is_packed, int mapped_device)
{

    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* trivial cases */
    if (op == MPI_NO_OP) {
        goto fn_exit;
    }

    /* error checking */
    MPIR_Assert(HANDLE_IS_BUILTIN(op));
    MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));

    bool use_yaksa = MPIR_Typerep_reduce_is_supported(op, source_dtp);
    if (use_yaksa) {
        int source_is_contig, target_is_contig;
        MPIR_Datatype_is_contig(source_dtp, &source_is_contig);
        MPIR_Datatype_is_contig(target_dtp, &target_is_contig);

        MPI_Aint true_extent, true_lb;
        if (source_is_packed) {
            mpi_errno = typerep_op_unpack(source_buf, target_buf, target_count, target_dtp,
                                          op, mapped_device);
        } else if (source_is_contig) {
            MPIR_Type_get_true_extent_impl(source_dtp, &true_lb, &true_extent);
            void *addr = MPIR_get_contig_ptr(source_buf, true_lb);
            mpi_errno = typerep_op_unpack(addr, target_buf, target_count, target_dtp,
                                          op, mapped_device);
        } else if (target_is_contig) {
            MPIR_Type_get_true_extent_impl(target_dtp, &true_lb, &true_extent);
            void *addr = MPIR_get_contig_ptr(target_buf, true_lb);
            mpi_errno = typerep_op_pack(source_buf, addr, source_count, source_dtp,
                                        op, mapped_device);
        } else {
            MPI_Aint data_sz;
            MPIR_Pack_size(source_count, source_dtp, &data_sz);
            void *src_ptr = MPL_malloc(data_sz, MPL_MEM_OTHER);
            MPI_Aint pack_size;
            MPIR_Typerep_pack(source_buf, source_count, source_dtp, 0, src_ptr, data_sz,
                              &pack_size, MPIR_TYPEREP_REQ_NULL);
            MPIR_Assert(pack_size == data_sz);
            mpi_errno = typerep_op_unpack(src_ptr, target_buf, target_count, target_dtp,
                                          op, mapped_device);
            MPL_free(src_ptr);
        }
    } else {
        mpi_errno = MPII_Typerep_op_fallback(source_buf, source_count, source_dtp,
                                             target_buf, target_count, target_dtp,
                                             op, source_is_packed);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int typerep_op_unpack(void *source_buf, void *target_buf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, int mapped_device)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t yaksa_type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_op_t yaksa_op = MPII_Typerep_get_yaksa_op(op);

    MPI_Aint data_sz;
    MPIR_Pack_size(count, datatype, &data_sz);

    yaksa_info_t info = NULL;
    if (mapped_device >= 0) {
        yaksa_info_create(&info);
        rc = yaksa_info_keyval_append(info, "yaksa_mapped_device", &mapped_device, sizeof(int));
        MPIR_Assert(rc == 0);
    }

    uintptr_t actual_bytes;
    yaksa_request_t request;
    rc = yaksa_iunpack(source_buf, data_sz, target_buf, count, yaksa_type, 0, &actual_bytes,
                       info, yaksa_op, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    MPIR_Assert(actual_bytes == data_sz);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int typerep_op_pack(void *source_buf, void *target_buf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Op op, int mapped_device)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t yaksa_type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_op_t yaksa_op = MPII_Typerep_get_yaksa_op(op);

    MPI_Aint data_sz;
    MPIR_Pack_size(count, datatype, &data_sz);

    yaksa_info_t info = NULL;
    if (mapped_device >= 0) {
        yaksa_info_create(&info);
        rc = yaksa_info_keyval_append(info, "yaksa_mapped_device", &mapped_device, sizeof(int));
        MPIR_Assert(rc == 0);
    }

    uintptr_t actual_pack_bytes;
    yaksa_request_t request;
    rc = yaksa_ipack(source_buf, count, yaksa_type, 0, target_buf, data_sz, &actual_pack_bytes,
                     info, yaksa_op, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    MPIR_Assert(actual_pack_bytes == data_sz);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_pack_stream(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                             MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                             MPI_Aint * actual_pack_bytes, void *stream)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    uintptr_t packed_bytes;;
    rc = yaksa_pack_stream(inbuf, incount, type, inoffset, outbuf, max_pack_bytes,
                           &packed_bytes, NULL, YAKSA_OP__REPLACE, stream);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    *actual_pack_bytes = packed_bytes;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_unpack_stream(const void *inbuf, MPI_Aint insize, void *outbuf,
                               MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                               MPI_Aint * actual_unpack_bytes, void *stream)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int rc;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    uintptr_t unpacked_bytes;;
    rc = yaksa_unpack_stream(inbuf, insize, outbuf, outcount, type, outoffset,
                             &unpacked_bytes, NULL, YAKSA_OP__REPLACE, stream);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    *actual_unpack_bytes = unpacked_bytes;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
