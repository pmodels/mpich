/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpir_dataloop.h>
#include <stdlib.h>

int MPIR_Type_to_iov(const void *buf, MPI_Aint count, MPI_Datatype type, MPI_Aint offset,
                     MPL_IOV * iov, int max_iov_len, MPI_Aint max_iov_bytes,
                     int *actual_iov_len, MPI_Aint * actual_iov_bytes)
{
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(buf, count, type);

    MPI_Aint last = offset + max_iov_bytes;
    *actual_iov_len = max_iov_len;
    MPIR_Segment_to_iov(seg, offset, &last, iov, actual_iov_len);
    *actual_iov_bytes = last - offset;

    MPIR_Segment_free(seg);

    return mpi_errno;
}

int MPIR_Type_to_iov_len(const void *buf, MPI_Aint count, MPI_Datatype type, MPI_Aint offset,
                         MPI_Aint max_iov_bytes, MPI_Aint * iov_len)
{
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(buf, count, type);

    MPI_Aint last = offset + max_iov_bytes;
    MPIR_Segment_count_contig_blocks(seg, offset, &last, iov_len);

    MPIR_Segment_free(seg);

    return mpi_errno;
}
