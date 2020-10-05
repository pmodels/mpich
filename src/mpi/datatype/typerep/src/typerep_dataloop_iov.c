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
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(buf, count, type);

    MPI_Aint last = byte_offset + max_iov_bytes;
    *actual_iov_len = max_iov_len;
    MPIR_Segment_to_iov(seg, byte_offset, &last, iov, (int *) actual_iov_len);
    *actual_iov_bytes = last - byte_offset;

    MPIR_Segment_free(seg);

    return mpi_errno;
}

int MPIR_Typerep_to_iov_offset(const void *buf, MPI_Aint count, MPI_Datatype type,
                               MPI_Aint iov_offset, struct iovec *iov, MPI_Aint max_iov_len,
                               MPI_Aint * actual_iov_len)
{
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(buf, count, type);

    MPI_Aint last;

    /* skip through IOV elements to get to the required IOV offset */
    MPI_Aint byte_offset = 0;
    MPI_Aint rem_iov_offset = iov_offset;
    while (rem_iov_offset) {
        last = MPIR_AINT_MAX;
        int iov_len = MPL_MIN(max_iov_len, rem_iov_offset);
        MPIR_Segment_to_iov(seg, byte_offset, &last, iov, &iov_len);
        rem_iov_offset += iov_len;
        for (int i = 0; i < iov_len; i++)
            byte_offset += iov[i].iov_len;
    }

    /* Final conversion to get the correct IOV set */
    last = MPIR_AINT_MAX;
    int iov_len = max_iov_len;
    MPIR_Segment_to_iov(seg, byte_offset, &last, iov, &iov_len);
    *actual_iov_len = (MPI_Aint) iov_len;

    MPIR_Segment_free(seg);

    return mpi_errno;
}

int MPIR_Typerep_iov_len(MPI_Aint count, MPI_Datatype type, MPI_Aint max_iov_bytes,
                         MPI_Aint * iov_len)
{
    MPIR_Segment *seg;
    int mpi_errno = MPI_SUCCESS;

    seg = MPIR_Segment_alloc(NULL, count, type);

    MPI_Aint last = max_iov_bytes;
    MPIR_Segment_count_contig_blocks(seg, 0, &last, iov_len);

    MPIR_Segment_free(seg);

    return mpi_errno;
}
