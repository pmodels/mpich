/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpir_typerep.h>
#include <dataloop.h>
#include <stdlib.h>

int MPIR_Typerep_to_iov(const void *buf, size_t count, MPI_Datatype type, size_t offset,
                        MPL_IOV * iov, int max_iov_len, size_t max_iov_bytes,
                        int *actual_iov_len, size_t * actual_iov_bytes)
{
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(buf, count, type);

    size_t last = offset + max_iov_bytes;
    *actual_iov_len = max_iov_len;
    MPIR_Segment_to_iov(seg, offset, &last, iov, actual_iov_len);
    *actual_iov_bytes = last - offset;

    MPIR_Segment_free(seg);

    return mpi_errno;
}

int MPIR_Typerep_iov_len(const void *buf, size_t count, MPI_Datatype type, size_t offset,
                         size_t max_iov_bytes, size_t * iov_len)
{
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(buf, count, type);

    size_t last = offset + max_iov_bytes;
    MPIR_Segment_count_contig_blocks(seg, offset, &last, iov_len);

    MPIR_Segment_free(seg);

    return mpi_errno;
}
