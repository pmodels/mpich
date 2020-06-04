/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"

int MPIR_Typerep_pack(const void *inbuf, MPI_Aint incount, MPI_Datatype datatype,
                      MPI_Aint inoffset, void *outbuf, MPI_Aint max_pack_bytes,
                      MPI_Aint * actual_pack_bytes)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_PACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_PACK);

    int mpi_errno = MPI_SUCCESS;
    int rc;

    /* fast-path for contig CPU buffers */
    bool is_contig;
    MPIR_Datatype_is_contig(datatype, &is_contig);
    if (is_contig) {
        MPL_pointer_attr_t inattr, outattr;
        MPL_gpu_query_pointer_attr((const char *) inbuf + inoffset, &inattr);
        MPL_gpu_query_pointer_attr(outbuf, &outattr);
        if ((inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
             inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
            (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
             outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
            MPI_Aint size;
            MPIR_Datatype_get_size_macro(datatype, size);
            MPI_Aint pack_bytes = MPL_MIN(incount * size - inoffset, max_pack_bytes);

            MPI_Aint true_lb;
            MPIR_Datatype_get_true_lb(datatype, &true_lb);

            MPIR_Memcpy(outbuf, (const char *) inbuf + inoffset + true_lb, pack_bytes);
            *actual_pack_bytes = pack_bytes;
            goto fn_exit;
        }
    }

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);

    yaksa_request_t request;
    uintptr_t real_pack_bytes;
    rc = yaksa_ipack(inbuf, incount, type, inoffset, outbuf, max_pack_bytes, &real_pack_bytes,
                     NULL, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = yaksa_request_wait(request);
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

    /* fast-path for contig CPU buffers */
    bool is_contig;
    MPIR_Datatype_is_contig(datatype, &is_contig);
    if (is_contig) {
        MPL_pointer_attr_t inattr, outattr;
        MPL_gpu_query_pointer_attr(inbuf, &inattr);
        MPL_gpu_query_pointer_attr((char *) outbuf + outoffset, &outattr);
        if ((inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
             inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
            (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
             outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
            MPI_Aint size;
            MPIR_Datatype_get_size_macro(datatype, size);
            MPI_Aint unpack_bytes = MPL_MIN(outcount * size - outoffset, insize);

            MPI_Aint true_lb;
            MPIR_Datatype_get_true_lb(datatype, &true_lb);

            MPIR_Memcpy((char *) outbuf + outoffset + true_lb, inbuf, unpack_bytes);
            *actual_unpack_bytes = unpack_bytes;
            goto fn_exit;
        }
    }

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(datatype);

    uintptr_t size;
    rc = yaksa_type_get_size(type, &size);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t real_insize = MPL_MIN(insize, size * outcount);

    yaksa_request_t request;
    uintptr_t real_unpack_bytes;
    rc = yaksa_iunpack(inbuf, real_insize, outbuf, outcount, type, outoffset, &real_unpack_bytes,
                       NULL, &request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    rc = yaksa_request_wait(request);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    *actual_unpack_bytes = (MPI_Aint) real_unpack_bytes;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_UNPACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
