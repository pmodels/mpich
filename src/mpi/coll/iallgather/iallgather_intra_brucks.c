/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Bruck's
 *
 * This algorithm is from the IEEE TPDS Nov 97 paper by Jehoshua Bruck
 * et al.  It is a variant of the disemmination algorithm for barrier.
 * It takes ceiling(lg p) steps.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 * where n is total size of data gathered on each process.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_sched_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, curr_cnt, rem, src, dst;
    int rank, comm_size;
    MPI_Aint recvtype_extent, recvtype_true_lb, recvtype_true_extent, recvbuf_extent;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     (comm_size * recvcount * recvtype_extent));

    /* allocate a temporary buffer of the same size as recvbuf. */
    /* get true extent of recvtype */

    recvbuf_extent = recvcount * comm_size * (MPL_MAX(recvtype_true_extent, recvtype_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, recvbuf_extent, mpi_errno, "tmp_buf",
                              MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - recvtype_true_lb);

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype, tmp_buf, recvcount, recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else {
        mpi_errno = MPIR_Sched_copy(((char *) recvbuf + rank * recvcount * recvtype_extent),
                                    recvcount, recvtype, tmp_buf, recvcount, recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* do the first \floor(\lg p) steps */

    curr_cnt = recvcount;
    pof2 = 1;
    while (pof2 <= comm_size / 2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        mpi_errno = MPIR_Sched_send(tmp_buf, curr_cnt, recvtype, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* logically sendrecv, so no barrier here */
        mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + curr_cnt * recvtype_extent),
                                    curr_cnt, recvtype, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        curr_cnt *= 2;
        pof2 *= 2;
    }

    /* if comm_size is not a power of two, one more step is needed */

    rem = comm_size - pof2;
    if (rem) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        mpi_errno = MPIR_Sched_send(tmp_buf, rem * recvcount, recvtype, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* logically sendrecv, so no barrier here */
        mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + curr_cnt * recvtype_extent),
                                    (rem * recvcount), recvtype, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    mpi_errno = MPIR_Sched_copy(tmp_buf, ((comm_size - rank) * recvcount), recvtype,
                                ((char *) recvbuf + rank * recvcount * recvtype_extent),
                                ((comm_size - rank) * recvcount), recvtype, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    if (rank) {
        mpi_errno =
            MPIR_Sched_copy(((char *) tmp_buf + (comm_size - rank) * recvcount * recvtype_extent),
                            rank * recvcount, recvtype, recvbuf, rank * recvcount, recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
