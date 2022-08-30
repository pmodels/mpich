/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Iallgatherv_intra_sched_brucks(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, j, i;
    MPI_Aint recvtype_extent, recvtype_sz;
    int dst, pof2, src, rem;
    void *tmp_buf;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);

    MPI_Aint total_count;
    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    /* allocate a temporary buffer of the same size as recvbuf. */
    tmp_buf = MPIR_Sched_alloc_state(s, total_count * recvtype_sz);
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    tmp_buf, recvcounts[rank] * recvtype_sz, MPI_BYTE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else {
        mpi_errno = MPIR_Sched_copy(((char *) recvbuf + displs[rank] * recvtype_extent),
                                    recvcounts[rank], recvtype,
                                    tmp_buf, recvcounts[rank] * recvtype_sz, MPI_BYTE, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* \floor(\lg p) send-recv rounds */
    /* recv these blocks (actual block numbers are mod comm_size):
     * - rank+1
     * - rank+2,rank+3
     * - rank+4,rank+5,rank+6,rank+7
     * - ...
     */
    /* curr_count is the amount of data (in counts of recvtype) that we have
     * right now, starting with the block we copied in the previous step */
    MPI_Aint curr_count;
    curr_count = recvcounts[rank];
    pof2 = 1;
    while (pof2 <= comm_size / 2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        MPI_Aint incoming_count = 0;
        for (i = 0; i < pof2; ++i) {
            incoming_count += recvcounts[(src + i) % comm_size];
        }

        mpi_errno = MPIR_Sched_send(tmp_buf, curr_count * recvtype_sz, MPI_BYTE, dst, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        /* sendrecv, no barrier */
        mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + curr_count * recvtype_sz),
                                    incoming_count * recvtype_sz, MPI_BYTE, src, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        /* we will send everything we had plus what we just got to next round's dst */
        curr_count += incoming_count;
        pof2 *= 2;
    }

    /* if comm_size is not a power of two, one more step is needed */
    rem = comm_size - pof2;
    if (rem) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        MPI_Aint cnt = 0;
        for (i = 0; i < rem; i++)
            cnt += recvcounts[(rank + i) % comm_size];

        mpi_errno = MPIR_Sched_send(tmp_buf, cnt * recvtype_sz, MPI_BYTE, dst, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        /* sendrecv, no barrier */
        mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + curr_count * recvtype_sz),
                                    (total_count - curr_count) * recvtype_sz, MPI_BYTE,
                                    src, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    MPI_Aint send_cnt;
    send_cnt = 0;
    for (i = 0; i < (comm_size - rank); i++) {
        j = (rank + i) % comm_size;
        mpi_errno = MPIR_Sched_copy(((char *) tmp_buf + send_cnt * recvtype_sz),
                                    recvcounts[j] * recvtype_sz, MPI_BYTE,
                                    ((char *) recvbuf + displs[j] * recvtype_extent),
                                    recvcounts[j], recvtype, s);
        MPIR_ERR_CHECK(mpi_errno);
        send_cnt += recvcounts[j];
    }

    for (i = 0; i < rank; i++) {
        mpi_errno = MPIR_Sched_copy(((char *) tmp_buf + send_cnt * recvtype_sz),
                                    recvcounts[i] * recvtype_sz, MPI_BYTE,
                                    ((char *) recvbuf + displs[i] * recvtype_extent),
                                    recvcounts[i], recvtype, s);
        MPIR_ERR_CHECK(mpi_errno);
        send_cnt += recvcounts[i];
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
