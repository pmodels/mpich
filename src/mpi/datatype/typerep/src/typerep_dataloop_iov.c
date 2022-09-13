/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpiimpl.h>
#include <mpir_typerep.h>
#include <dataloop.h>
#include <stdlib.h>

int MPIR_Typerep_to_iov(const void *buf, MPI_Aint count, MPI_Datatype type, MPI_Aint byte_offset,
                        struct iovec *iov, MPI_Aint max_iov_len, MPI_Aint max_iov_bytes,
                        MPI_Aint * actual_iov_len, MPI_Aint * actual_iov_bytes)
{
    int mpi_errno = MPI_SUCCESS;

    if (max_iov_len == 0 || max_iov_bytes == 0) {
        *actual_iov_len = 0;
        *actual_iov_bytes = 0;
    } else {
        MPIR_Segment *seg;
        seg = MPIR_Segment_alloc(buf, count, type);

        MPI_Aint last = byte_offset + max_iov_bytes;
        *actual_iov_len = max_iov_len;
        MPIR_Segment_to_iov(seg, byte_offset, &last, iov, (int *) actual_iov_len);
        *actual_iov_bytes = last - byte_offset;

        MPIR_Segment_free(seg);
    }

    return mpi_errno;
}

int MPIR_Typerep_to_iov_offset(const void *buf, MPI_Aint count, MPI_Datatype type,
                               MPI_Aint iov_offset, struct iovec *iov, MPI_Aint max_iov_len,
                               MPI_Aint * actual_iov_len)
{
    int mpi_errno = MPI_SUCCESS;

    if (HANDLE_IS_BUILTIN(type)) {
        MPI_Aint typesize;
        typesize = MPIR_Datatype_get_basic_size(type);
        if (max_iov_len >= 1) {
            iov[0].iov_base = (char *) buf;
            iov[0].iov_len = typesize;
            *actual_iov_len = 1;
        } else {
            *actual_iov_len = 0;
        }
    } else {
        MPIR_Datatype *dt_ptr;
        MPIR_Datatype_get_ptr(type, dt_ptr);

        mpi_errno = MPIR_Dataloop_iov(buf, count, dt_ptr->typerep.handle, dt_ptr->extent,
                                      iov_offset, iov, max_iov_len, actual_iov_len);
    }
    return mpi_errno;
}

int MPIR_Typerep_iov_len(MPI_Aint count, MPI_Datatype type, MPI_Aint max_iov_bytes,
                         MPI_Aint * iov_len, MPI_Aint * actual_iov_bytes)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Datatype *dt_ptr = NULL;
    int is_contig;
    MPI_Aint type_size, num_contig;

    if (HANDLE_IS_BUILTIN(type)) {
        is_contig = 1;
        type_size = MPIR_Datatype_get_basic_size(type);
        num_contig = 1;
    } else {
        MPIR_Datatype_get_ptr(type, dt_ptr);
        is_contig = dt_ptr->is_contig;
        type_size = dt_ptr->size;
        num_contig = dt_ptr->typerep.num_contig_blocks;
    }

    /* -1 means to get the whole range */
    if (max_iov_bytes == -1) {
        max_iov_bytes = count * type_size;
    }

    if (max_iov_bytes >= count * type_size) {
        *iov_len = count * num_contig;
        if (actual_iov_bytes) {
            *actual_iov_bytes = count * type_size;
        }
        goto fn_exit;
    }

    if (is_contig) {
        /* only whole segment */
        *iov_len = 0;
        if (actual_iov_bytes) {
            *actual_iov_bytes = 0;
        }
        goto fn_exit;
    }

    MPI_Aint rem_bytes;
    rem_bytes = max_iov_bytes;
    *iov_len = 0;

    MPI_Aint n;
    n = rem_bytes / type_size;
    rem_bytes -= n * type_size;
    *iov_len += n * num_contig;

    if (num_contig > 1) {
        mpi_errno = MPIR_Dataloop_iov_len(dt_ptr->typerep.handle, &rem_bytes, iov_len);
        MPIR_ERR_CHECK(mpi_errno);
    }
    if (actual_iov_bytes) {
        *actual_iov_bytes = max_iov_bytes - rem_bytes;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
