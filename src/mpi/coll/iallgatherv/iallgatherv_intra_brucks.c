/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_sched_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int recvcounts[], const int displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, j, i;
    MPI_Aint recvbuf_extent, recvtype_extent, recvtype_true_extent, recvtype_true_lb;
    int send_cnt, dst, total_count, pof2, src, rem;
    int incoming_count, curr_count;
    void *tmp_buf;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    /* allocate a temporary buffer of the same size as recvbuf. */

    /* get true extent of recvtype */
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(recvtype_true_extent, recvtype_extent));
    recvbuf_extent = total_count * (MPL_MAX(recvtype_true_extent, recvtype_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, recvbuf_extent, mpi_errno, "tmp_buf",
                              MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - recvtype_true_lb);

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    tmp_buf, recvcounts[rank], recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else {
        mpi_errno = MPIR_Sched_copy(((char *) recvbuf + displs[rank] * recvtype_extent),
                                    recvcounts[rank], recvtype,
                                    tmp_buf, recvcounts[rank], recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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
    curr_count = recvcounts[rank];
    pof2 = 1;
    while (pof2 <= comm_size / 2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        incoming_count = 0;
        for (i = 0; i < pof2; ++i) {
            incoming_count += recvcounts[(src + i) % comm_size];
        }

        mpi_errno = MPIR_Sched_send(tmp_buf, curr_count, recvtype, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier */
        mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + curr_count * recvtype_extent),
                                    incoming_count, recvtype, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
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

        send_cnt = 0;
        for (i = 0; i < rem; i++)
            send_cnt += recvcounts[(rank + i) % comm_size];

        mpi_errno = MPIR_Sched_send(tmp_buf, send_cnt, recvtype, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier */
        mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + curr_count * recvtype_extent),
                                    (total_count - curr_count), recvtype, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    send_cnt = 0;
    for (i = 0; i < (comm_size - rank); i++) {
        j = (rank + i) % comm_size;
        mpi_errno = MPIR_Sched_copy(((char *) tmp_buf + send_cnt * recvtype_extent),
                                    recvcounts[j], recvtype,
                                    ((char *) recvbuf + displs[j] * recvtype_extent),
                                    recvcounts[j], recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        send_cnt += recvcounts[j];
    }

    for (i = 0; i < rank; i++) {
        mpi_errno = MPIR_Sched_copy(((char *) tmp_buf + send_cnt * recvtype_extent),
                                    recvcounts[i], recvtype,
                                    ((char *) recvbuf + displs[i] * recvtype_extent),
                                    recvcounts[i], recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        send_cnt += recvcounts[i];
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
