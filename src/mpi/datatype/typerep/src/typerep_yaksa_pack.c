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

/* When a returned typerep_req is expected, using the nonblocking yaksa routine and
 * return the request; otherwise use the blocking yaksa routine. */
static int typerep_do_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                           MPIR_Typerep_req * typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_TYPEREP_DO_COPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_TYPEREP_DO_COPY);

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

    if ((inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
         inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
        (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
         outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_TYPEREP_DO_COPY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* When a returned typerep_req is expected, using the nonblocking yaksa routine and
 * return the request; otherwise use the blocking yaksa routine. */
static int typerep_do_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                           MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                           MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_TYPEREP_DO_PACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_TYPEREP_DO_PACK);

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
    const void *inbuf_ptr;      /* adjusted by true_lb */
    MPI_Aint total_size = 0;
    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
        inbuf_ptr = inbuf;
        total_size = incount * MPIR_Datatype_get_basic_size(datatype);
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(datatype, dtp);
        is_contig = dtp->is_contig;
        inbuf_ptr = (const char *) inbuf + dtp->true_lb;
        total_size = incount * dtp->size;
    }

    MPL_pointer_attr_t inattr, outattr;
    MPIR_GPU_query_pointer_attr(inbuf_ptr, &inattr);
    MPIR_GPU_query_pointer_attr(outbuf, &outattr);

    if (is_contig &&
        (inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
         inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
        (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
         outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
        *actual_pack_bytes = MPL_MIN(total_size - inoffset, max_pack_bytes);
        MPIR_Memcpy(outbuf, (const char *) inbuf_ptr + inoffset, *actual_pack_bytes);
        goto fn_exit;
    }

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_info_t info = MPII_yaksa_get_info(&inattr, &outattr);

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

    rc = MPII_yaksa_free_info(info);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_pack_bytes = (MPI_Aint) real_pack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_TYPEREP_DO_PACK);
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

static int op_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                   MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                   MPI_Aint * actual_pack_bytes, MPI_Op op)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_TYPEREP_OP_PACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_TYPEREP_OP_PACK);

    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (incount == 0) {
        *actual_pack_bytes = 0;
        goto fn_exit;
    }

    MPIR_Assert(datatype != MPI_DATATYPE_NULL);

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_op_t yaksa_op = MPII_Typerep_get_yaksa_op(op);

    yaksa_request_t request;
    uintptr_t real_pack_bytes;
    rc = yaksa_ipack(inbuf, incount, type, inoffset, outbuf, max_pack_bytes, &real_pack_bytes,
                     NULL, yaksa_op, &request);

    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_pack_bytes = (MPI_Aint) real_pack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_TYPEREP_OP_PACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* When a returned typerep_req is expected, using the nonblocking yaksa routine and
 * return the request; otherwise use the blocking yaksa routine. */
static int typerep_do_unpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                             MPI_Datatype datatype, MPI_Aint outoffset,
                             MPI_Aint * actual_unpack_bytes, MPIR_Typerep_req * typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_TYPEREP_DO_UNPACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_TYPEREP_DO_UNPACK);

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
    const void *outbuf_ptr;     /* adjusted by true_lb */
    MPI_Aint total_size = 0;
    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
        outbuf_ptr = outbuf;
        total_size = outcount * MPIR_Datatype_get_basic_size(datatype);
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(datatype, dtp);
        is_contig = dtp->is_contig;
        outbuf_ptr = (char *) outbuf + dtp->true_lb;
        total_size = outcount * dtp->size;
    }

    MPL_pointer_attr_t inattr, outattr;
    MPIR_GPU_query_pointer_attr(inbuf, &inattr);
    MPIR_GPU_query_pointer_attr(outbuf_ptr, &outattr);

    if (is_contig &&
        (inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
         inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
        (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
         outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
        *actual_unpack_bytes = MPL_MIN(total_size - outoffset, insize);
        MPIR_Memcpy((char *) outbuf_ptr + outoffset, inbuf, *actual_unpack_bytes);
        goto fn_exit;
    }

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_info_t info = MPII_yaksa_get_info(&inattr, &outattr);

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

    rc = MPII_yaksa_free_info(info);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_unpack_bytes = (MPI_Aint) real_unpack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_TYPEREP_DO_UNPACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int op_unpack(const void *inbuf, MPI_Aint insize, void *outbuf,
                     MPI_Aint outcount, MPI_Datatype datatype, MPI_Aint outoffset,
                     MPI_Aint * actual_unpack_bytes, MPI_Op op)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_TYPEREP_OP_UNPACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_TYPEREP_OP_UNPACK);

    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (outcount == 0) {
        *actual_unpack_bytes = 0;
        goto fn_exit;
    }

    MPIR_Assert(datatype != MPI_DATATYPE_NULL);

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);
    yaksa_op_t yaksa_op = MPII_Typerep_get_yaksa_op(op);

    uintptr_t size;
    rc = yaksa_type_get_size(type, &size);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t real_insize = MPL_MIN(insize, size * outcount);

    yaksa_request_t request;
    uintptr_t real_unpack_bytes;
    rc = yaksa_iunpack(inbuf, real_insize, outbuf, outcount, type, outoffset, &real_unpack_bytes,
                       NULL, yaksa_op, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_unpack_bytes = (MPI_Aint) real_unpack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_TYPEREP_OP_UNPACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_icopy(void *outbuf, const void *inbuf, MPI_Aint num_bytes,
                       MPIR_Typerep_req * typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_ICOPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_ICOPY);

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_copy(outbuf, inbuf, num_bytes, typerep_req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_ICOPY);
    return mpi_errno;
}

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_COPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_COPY);

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_copy(outbuf, inbuf, num_bytes, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_COPY);
    return mpi_errno;
}

int MPIR_Typerep_ipack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                       MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                       MPI_Aint * actual_pack_bytes, MPIR_Typerep_req * typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_IPACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_IPACK);

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_pack(inbuf, incount, datatype, inoffset, outbuf, max_pack_bytes,
                                actual_pack_bytes, typerep_req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_IPACK);
    return mpi_errno;
}

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_PACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_PACK);

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_pack(inbuf, incount, datatype, inoffset, outbuf, max_pack_bytes,
                                actual_pack_bytes, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_PACK);
    return mpi_errno;
}

int MPIR_Typerep_iunpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                         MPI_Datatype datatype, MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes,
                         MPIR_Typerep_req * typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_IUNPACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_IUNPACK);

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_unpack(inbuf, insize, outbuf, outcount, datatype, outoffset,
                                  actual_unpack_bytes, typerep_req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_IUNPACK);
    return mpi_errno;
}

int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                        MPI_Datatype datatype, MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_UNPACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_UNPACK);

    int mpi_errno = MPI_SUCCESS;
    mpi_errno = typerep_do_unpack(inbuf, insize, outbuf, outcount, datatype, outoffset,
                                  actual_unpack_bytes, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_UNPACK);
    return mpi_errno;
}

int MPIR_Typerep_wait(MPIR_Typerep_req typerep_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_WAIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_WAIT);
    int mpi_errno = MPI_SUCCESS;
    int rc;

    if (typerep_req == MPIR_TYPEREP_REQ_NULL)
        goto fn_exit;

    rc = yaksa_request_wait((yaksa_request_t) typerep_req);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_WAIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_op(void *source_buf, int source_count, MPI_Datatype source_dtp,
                    void *target_buf, int target_count, MPI_Datatype target_dtp,
                    MPI_Op op, int src_kind)
{

    int mpi_errno = MPI_SUCCESS;
    MPIR_op_function *uop = NULL;
    MPI_Aint source_dtp_size = 0, source_dtp_extent = 0, max_pack_bytes, actual_pack_bytes;
    int is_empty_source = FALSE;
    int is_supported = 0;
    struct iovec *typerep_vec = NULL;
    int i;
    MPI_Aint vec_len = 0, type_extent = 0, type_size = 0, src_type_stride = 0;
    MPI_Datatype type;
    MPIR_Datatype *dtp;
    MPI_Aint curr_len;
    void *curr_loc;
    int accumulated_count;
    char *src_ptr = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_OP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_OP);

    /* first Judge if source buffer is empty */
    if (op == MPI_NO_OP)
        is_empty_source = TRUE;

    if (is_empty_source == FALSE) {
        MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));
        MPIR_Datatype_get_size_macro(source_dtp, source_dtp_size);
        MPIR_Datatype_get_extent_macro(source_dtp, source_dtp_extent);
    }

    if ((HANDLE_IS_BUILTIN(op))
        && ((*MPIR_OP_HDL_TO_DTYPE_FN(op)) (source_dtp) == MPI_SUCCESS)) {
        is_supported = MPIR_Typerep_reduce_is_supported(op, source_dtp);
    } else {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         __func__, __LINE__, MPI_ERR_OP,
                                         "**opnotpredefined", "**opnotpredefined %d", op);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }

    if (!(is_empty_source == TRUE || HANDLE_IS_BUILTIN(target_dtp))) {
        MPIR_Datatype_get_ptr(target_dtp, dtp);
        MPIR_Assert(dtp != NULL);
        vec_len = dtp->typerep.num_contig_blocks * target_count + 1;
        /* +1 needed because Rob says so */
        typerep_vec = (struct iovec *)
            MPL_malloc(vec_len * sizeof(struct iovec), MPL_MEM_OTHER);
        /* --BEGIN ERROR HANDLING-- */
        if (!typerep_vec) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        MPI_Aint actual_iov_len, actual_iov_bytes;
        MPIR_Typerep_to_iov(NULL, target_count, target_dtp, 0, typerep_vec, vec_len,
                            source_count * source_dtp_size, &actual_iov_len, &actual_iov_bytes);
        vec_len = actual_iov_len;

        type = dtp->basic_type;
        MPIR_Assert(type != MPI_DATATYPE_NULL);

        MPIR_Assert(type == source_dtp);
        type_size = source_dtp_size;
        type_extent = source_dtp_extent;
        /* If the source buffer has been packed by the caller, the distance between
         * two elements can be smaller than extent. E.g., predefined pairtype may
         * have larger extent than size.*/
        /* when predefined pairtype have larger extent than size, we'll end up
         * missaligned access. Memcpy the source to workaround the alignment issue.
         */
        if (src_kind == MPIDIG_ACC_SRCBUF_PACKED) {
            src_type_stride = source_dtp_size;
            if (source_dtp_size < source_dtp_extent) {
                src_ptr = MPL_malloc(source_dtp_extent, MPL_MEM_OTHER);
            }
        } else {
            src_type_stride = source_dtp_extent;
        }
    }
    if (is_supported) {
        if (is_empty_source == TRUE || HANDLE_IS_BUILTIN(target_dtp)) {
            /* directly apply op if target dtp is predefined dtp OR source buffer is empty */
            max_pack_bytes = source_dtp_size * source_count;
            mpi_errno =
                op_pack(source_buf, source_count, source_dtp, 0, target_buf,
                        max_pack_bytes, &actual_pack_bytes, op);

        } else {
            /* derived datatype */
            i = 0;
            curr_loc = typerep_vec[0].iov_base;
            curr_len = typerep_vec[0].iov_len;
            accumulated_count = 0;
            while (i != vec_len) {
                if (curr_len < type_size) {
                    MPIR_Assert(i != vec_len);
                    i++;
                    curr_len += typerep_vec[i].iov_len;
                    continue;
                }

                MPI_Aint count;
                MPIR_Assign_trunc(count, curr_len / type_size, MPI_Aint);

                if (src_ptr) {
                    MPI_Aint unpacked_size;
                    op_unpack((char *) source_buf +
                              src_type_stride * accumulated_count, source_dtp_size,
                              src_ptr, 1, source_dtp, 0, &unpacked_size, op);
                    MPIR_Typerep_copy((char *) target_buf + MPIR_Ptr_to_aint(curr_loc), src_ptr,
                                      type_size * count);
                } else {
                    max_pack_bytes = type_size * count;
                    if (source_dtp_size == source_dtp_extent) {
                        mpi_errno =
                            op_pack((char *) source_buf +
                                    src_type_stride * accumulated_count, count,
                                    type, 0,
                                    (char *) target_buf +
                                    MPIR_Ptr_to_aint(curr_loc), max_pack_bytes,
                                    &actual_pack_bytes, op);
                    } else {
                        char *ptr = MPL_malloc(source_dtp_extent, MPL_MEM_OTHER);
                        MPI_Aint unpacked_size;
                        mpi_errno =
                            op_pack((char *) source_buf +
                                    src_type_stride * accumulated_count, count,
                                    type, 0, ptr, max_pack_bytes, &actual_pack_bytes, op);
                        MPIR_Typerep_unpack(ptr, source_dtp_size,
                                            (char *) target_buf + MPIR_Ptr_to_aint(curr_loc), 1,
                                            source_dtp, 0, &unpacked_size);
                        MPL_free(ptr);
                    }
                }
                if (curr_len % type_size == 0) {
                    i++;
                    if (i != vec_len) {
                        curr_loc = typerep_vec[i].iov_base;
                        curr_len = typerep_vec[i].iov_len;
                    }
                } else {
                    curr_loc = (void *) ((char *) curr_loc + type_extent * count);
                    curr_len -= type_size * count;
                }

                accumulated_count += count;
            }
            MPL_free(src_ptr);
            MPL_free(typerep_vec);
        }
    } else {
        if ((HANDLE_IS_BUILTIN(op))
            && ((*MPIR_OP_HDL_TO_DTYPE_FN(op)) (source_dtp) == MPI_SUCCESS)) {
            /* get the function by indexing into the op table */
            uop = MPIR_OP_HDL_TO_FN(op);
        }
        void *save_targetbuf = NULL;
        void *host_targetbuf = NULL;

        host_targetbuf = MPIR_gpu_host_swap(target_buf, target_count, target_dtp);

        if (host_targetbuf) {
            save_targetbuf = target_buf;
            target_buf = host_targetbuf;
        }

        if (is_empty_source == TRUE || HANDLE_IS_BUILTIN(target_dtp)) {
            /* directly apply op if target dtp is predefined dtp OR source buffer is empty */
            MPI_Aint tmp_count = source_count;
            (*uop) (source_buf, target_buf, &tmp_count, &source_dtp);
        } else {
            i = 0;
            curr_loc = typerep_vec[0].iov_base;
            curr_len = typerep_vec[0].iov_len;
            accumulated_count = 0;
            while (i != vec_len) {
                if (curr_len < type_size) {
                    MPIR_Assert(i != vec_len);
                    i++;
                    curr_len += typerep_vec[i].iov_len;
                    continue;
                }

                MPI_Aint count;
                MPIR_Assign_trunc(count, curr_len / type_size, MPI_Aint);

                if (src_ptr) {
                    MPI_Aint unpacked_size;
                    MPIR_Typerep_unpack((char *) source_buf + src_type_stride * accumulated_count,
                                        source_dtp_size, src_ptr, 1, source_dtp, 0, &unpacked_size);
                    (*uop) (src_ptr, (char *) target_buf + MPIR_Ptr_to_aint(curr_loc), &count,
                            &type);
                } else {
                    (*uop) ((char *) source_buf + src_type_stride * accumulated_count,
                            (char *) target_buf + MPIR_Ptr_to_aint(curr_loc), &count, &type);
                }

                if (curr_len % type_size == 0) {
                    i++;
                    if (i != vec_len) {
                        curr_loc = typerep_vec[i].iov_base;
                        curr_len = typerep_vec[i].iov_len;
                    }
                } else {
                    curr_loc = (void *) ((char *) curr_loc + type_extent * count);
                    curr_len -= type_size * count;
                }

                accumulated_count += count;
            }

            MPL_free(src_ptr);
            MPL_free(typerep_vec);
        }

        if (save_targetbuf) {
            MPIR_gpu_swap_back(target_buf, save_targetbuf, target_count, target_dtp);
            target_buf = save_targetbuf;
        }
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_OP);
    return mpi_errno;
}
