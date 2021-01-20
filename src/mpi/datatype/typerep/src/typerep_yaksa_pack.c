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
        rc = yaksa_ipack(inbuf, num_bytes, YAKSA_TYPE__BYTE, 0, outbuf, num_bytes,
                         &actual_pack_bytes, NULL, &request);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
        MPIR_Assert(actual_pack_bytes == num_bytes);

        rc = yaksa_request_wait(request);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_COPY);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

typedef enum {
    MEMCPY_DIR__PACK,
    MEMCPY_DIR__UNPACK,
} memcpy_dir_e;

static inline bool fastpath_memcpy(const void *inbuf, void *outbuf, MPI_Datatype type,
                                   MPI_Aint count, MPI_Aint offset, MPI_Aint max_bytes,
                                   MPI_Aint * actual_bytes, memcpy_dir_e dir)
{
    bool ret = false;

    /* special case builtin types, over other contiguous types, to
     * avoid the three pointer dereferences (for is_contig, size and
     * true_lb) into the MPIR_Datatype structure */
    if (HANDLE_IS_BUILTIN(type)) {
        MPL_pointer_attr_t inattr, outattr;
        MPIR_GPU_query_pointer_attr(inbuf, &inattr);
        MPIR_GPU_query_pointer_attr(outbuf, &outattr);

        if ((inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
             inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
            (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
             outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
            MPI_Aint size = MPIR_Datatype_get_basic_size(type);
            *actual_bytes = MPL_MIN(count * size - offset, max_bytes);
            if (dir == MEMCPY_DIR__PACK)
                MPIR_Memcpy(outbuf, (const char *) inbuf + offset, *actual_bytes);
            else
                MPIR_Memcpy((char *) outbuf + offset, inbuf, *actual_bytes);
            ret = true;
        }
    } else {
        MPIR_Datatype *dtp;
        MPIR_Datatype_get_ptr(type, dtp);

        if (dtp->is_contig) {
            MPL_pointer_attr_t inattr, outattr;

            if (dir == MEMCPY_DIR__PACK) {
                MPIR_GPU_query_pointer_attr((const char *) inbuf + dtp->true_lb + offset, &inattr);
                MPIR_GPU_query_pointer_attr(outbuf, &outattr);
            } else {
                MPIR_GPU_query_pointer_attr(inbuf, &inattr);
                MPIR_GPU_query_pointer_attr((char *) outbuf + dtp->true_lb + offset, &outattr);
            }

            if ((inattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
                 inattr.type == MPL_GPU_POINTER_REGISTERED_HOST) &&
                (outattr.type == MPL_GPU_POINTER_UNREGISTERED_HOST ||
                 outattr.type == MPL_GPU_POINTER_REGISTERED_HOST)) {
                *actual_bytes = MPL_MIN(count * dtp->size - offset, max_bytes);
                if (dir == MEMCPY_DIR__PACK)
                    MPIR_Memcpy(outbuf, (const char *) inbuf + dtp->true_lb + offset,
                                *actual_bytes);
                else
                    MPIR_Memcpy((char *) outbuf + dtp->true_lb + offset, inbuf, *actual_bytes);
                ret = true;
            }
        }
    }

    return ret;
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

    /* fast-path for contig CPU buffers */
    bool fast = fastpath_memcpy(inbuf, outbuf, datatype, incount, inoffset, max_pack_bytes,
                                actual_pack_bytes, MEMCPY_DIR__PACK);
    if (fast)
        goto fn_exit;

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

    if (insize == 0) {
        *actual_unpack_bytes = 0;
        goto fn_exit;
    }

    MPIR_Assert(datatype != MPI_DATATYPE_NULL);

    /* fast-path for contig CPU buffers */
    bool fast = fastpath_memcpy(inbuf, outbuf, datatype, outcount, outoffset, insize,
                                actual_unpack_bytes, MEMCPY_DIR__UNPACK);
    if (fast)
        goto fn_exit;

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
