/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Bruck's Algorithm:
 *
 * This algorithm is from the IEEE TPDS Nov 97 paper by Jehoshua Bruck
 * et al.  It is a variant of the disemmination algorithm for barrier.
 * It takes ceiling(lg p) steps.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 * where n is total size of data gathered on each process.
 */

int MPIR_Allgatherv_intra_brucks(const void *sendbuf,
                                 MPI_Aint sendcount,
                                 MPI_Datatype sendtype,
                                 void *recvbuf,
                                 const MPI_Aint * recvcounts,
                                 const MPI_Aint * displs,
                                 MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, rank, j, i;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint recvtype_extent, recvtype_sz;
    int pof2, src, rem;
    MPI_Aint curr_cnt, send_cnt, recv_cnt, total_count;
    int dst;
    void *tmp_buf;
    MPIR_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* allocate a temporary buffer that can hold all the data */
    MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, total_count * recvtype_sz, mpi_errno, "tmp_buf",
                        MPL_MEM_BUFFER);

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   tmp_buf, recvcounts[rank] * recvtype_sz, MPI_BYTE);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Localcopy(((char *) recvbuf +
                                    displs[rank] * recvtype_extent),
                                   recvcounts[rank], recvtype, tmp_buf,
                                   recvcounts[rank] * recvtype_sz, MPI_BYTE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* do the first \floor(\lg p) steps */

    curr_cnt = recvcounts[rank];
    pof2 = 1;
    while (pof2 <= comm_size / 2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        mpi_errno = MPIC_Sendrecv(tmp_buf, curr_cnt * recvtype_sz, MPI_BYTE, dst,
                                  MPIR_ALLGATHERV_TAG,
                                  ((char *) tmp_buf + curr_cnt * recvtype_sz),
                                  (total_count - curr_cnt) * recvtype_sz, MPI_BYTE,
                                  src, MPIR_ALLGATHERV_TAG, comm_ptr, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            recv_cnt = 0;
        } else
            MPIR_Get_count_impl(&status, recvtype, &recv_cnt);
        curr_cnt += recv_cnt;

        pof2 *= 2;
    }

    /* if comm_size is not a power of two, one more step is needed */

    rem = comm_size - pof2;
    if (rem) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        send_cnt = 0;
        for (i = 0; i < rem; i++)
            send_cnt += recvcounts[(rank + i) % comm_size];

        mpi_errno = MPIC_Sendrecv(tmp_buf, send_cnt * recvtype_sz, MPI_BYTE,
                                  dst, MPIR_ALLGATHERV_TAG,
                                  ((char *) tmp_buf + curr_cnt * recvtype_sz),
                                  (total_count - curr_cnt) * recvtype_sz, MPI_BYTE,
                                  src, MPIR_ALLGATHERV_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    send_cnt = 0;
    for (i = 0; i < (comm_size - rank); i++) {
        j = (rank + i) % comm_size;
        mpi_errno = MPIR_Localcopy((char *) tmp_buf + send_cnt * recvtype_sz,
                                   recvcounts[j] * recvtype_sz, MPI_BYTE,
                                   (char *) recvbuf + displs[j] * recvtype_extent,
                                   recvcounts[j], recvtype);
        MPIR_ERR_CHECK(mpi_errno);
        send_cnt += recvcounts[j];
    }

    for (i = 0; i < rank; i++) {
        mpi_errno = MPIR_Localcopy((char *) tmp_buf + send_cnt * recvtype_sz,
                                   recvcounts[i] * recvtype_sz, MPI_BYTE,
                                   (char *) recvbuf + displs[i] * recvtype_extent,
                                   recvcounts[i], recvtype);
        MPIR_ERR_CHECK(mpi_errno);
        send_cnt += recvcounts[i];
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
