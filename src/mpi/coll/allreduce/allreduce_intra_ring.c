/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory.
 *
 */
/* Routine to schedule a ring exchange based allreduce. The algorithm is
 * based on Baidu's ring based allreduce. http://andrew.gibiansky.com/ */

#include "mpiimpl.h"

int MPIR_Allreduce_intra_ring(const void *sendbuf, void *recvbuf, MPI_Aint count,
                              MPI_Datatype datatype, MPI_Op op,
                              MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int i, src, dst;
    int nranks, is_inplace, rank;
    size_t extent;
    MPI_Aint lb, true_extent;
    MPI_Aint *cnts, *displs;    /* Created for the allgatherv call */
    int send_rank, recv_rank, total_count;
    void *tmpbuf;
    int tag;
    MPIR_Request *reqs[2];      /* one send and one recv per transfer */

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    cnts = (MPI_Aint *) MPL_malloc(nranks * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!cnts, mpi_errno, MPI_ERR_OTHER, "**nomem");
    displs = (MPI_Aint *) MPL_malloc(nranks * sizeof(MPI_Aint), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!displs, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (i = 0; i < nranks; i++)
        cnts[i] = 0;

    total_count = 0;
    for (i = 0; i < nranks; i++) {
        cnts[i] = (count + nranks - 1) / nranks;
        if (total_count + cnts[i] > count) {
            cnts[i] = count - total_count;
            break;
        } else
            total_count += cnts[i];
    }

    displs[0] = 0;
    for (i = 1; i < nranks; i++)
        displs[i] = displs[i - 1] + cnts[i - 1];

    /* Phase 1: copy to tmp buf */
    if (!is_inplace) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    /* Phase 2: Ring based send recv reduce scatter */
    /* Need only 2 spaces for current and previous reduce_id(s) */
    tmpbuf = MPL_malloc(count * extent, MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!tmpbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    src = (nranks + rank - 1) % nranks;
    dst = (rank + 1) % nranks;

    for (i = 0; i < nranks - 1; i++) {
        recv_rank = (nranks + rank - 2 - i) % nranks;
        send_rank = (nranks + rank - 1 - i) % nranks;

        /* get a new tag to prevent out of order messages */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIC_Irecv(tmpbuf, cnts[recv_rank], datatype, src, tag, comm, &reqs[0]);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }


        mpi_errno = MPIC_Isend((char *) recvbuf + displs[send_rank] * extent, cnts[send_rank],
                               datatype, dst, tag, comm, &reqs[1], errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno = MPIC_Waitall(2, reqs, MPI_STATUSES_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }


        mpi_errno =
            MPIR_Reduce_local(tmpbuf, (char *) recvbuf + displs[recv_rank] * extent,
                              cnts[recv_rank], datatype, op);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Phase 3: Allgatherv ring, so everyone has the reduced data */
    mpi_errno = MPIR_Allgatherv_intra_ring(MPI_IN_PLACE, -1, MPI_DATATYPE_NULL, recvbuf, cnts,
                                           displs, datatype, comm, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    MPL_free(cnts);
    MPL_free(displs);
    MPL_free(tmpbuf);

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
