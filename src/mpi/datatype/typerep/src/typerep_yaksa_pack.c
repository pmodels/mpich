/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"

int MPIR_Typerep_copy(void *outbuf, const void *inbuf, MPI_Aint num_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_COPY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_COPY);

    int mpi_errno = MPI_SUCCESS;
    int rc;

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
        yaksa_request_t request;
        uintptr_t actual_pack_bytes;

        yaksa_info_t info = MPII_yaksa_get_info(&inattr, &outattr);
        rc = yaksa_ipack(inbuf, num_bytes, YAKSA_TYPE__BYTE, 0, outbuf, num_bytes,
                         &actual_pack_bytes, info, YAKSA_OP__REPLACE, &request);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
        MPIR_Assert(actual_pack_bytes == num_bytes);

        rc = yaksa_request_wait(request);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

        rc = MPII_yaksa_free_info(info);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_COPY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_PACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_PACK);

    int mpi_errno = MPI_SUCCESS;
    int rc;

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

    yaksa_request_t request;
    uintptr_t real_pack_bytes;
    rc = yaksa_ipack(inbuf, incount, type, inoffset, outbuf, max_pack_bytes, &real_pack_bytes,
                     info, YAKSA_OP__REPLACE, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = MPII_yaksa_free_info(info);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_pack_bytes = (MPI_Aint) real_pack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_PACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_unpack(const void *inbuf, MPI_Aint insize, void *outbuf, MPI_Aint outcount,
                        MPI_Datatype datatype, MPI_Aint outoffset, MPI_Aint * actual_unpack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_UNPACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_UNPACK);

    int mpi_errno = MPI_SUCCESS;
    int rc;

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

    yaksa_request_t request;
    uintptr_t real_unpack_bytes;
    rc = yaksa_iunpack(inbuf, real_insize, outbuf, outcount, type, outoffset, &real_unpack_bytes,
                       info, YAKSA_OP__REPLACE, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = MPII_yaksa_free_info(info);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_unpack_bytes = (MPI_Aint) real_unpack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_UNPACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
