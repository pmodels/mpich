/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Iallreduce_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                   MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, rem, comm_size, is_commutative, rank;
    int newrank, mask, newdst, dst;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store incoming data */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    tmp_buf = MPIR_Sched_alloc_state(s, count * (MPL_MAX(extent, true_extent)));
    MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        MPIR_ERR_CHECK(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm_ptr->coll.pof2;

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, rank + 1, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = -1;
        } else {        /* odd */
            mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, rank - 1, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        mask = 0x1;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            /* Send the most current data, which is in recvbuf. Recv
             * into tmp_buf */
            mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, dst, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* tmp_buf contains data received in this step.
             * recvbuf contains data accumulated so far */

            if (is_commutative || (dst < rank)) {
                /* op is commutative OR the order is already right */
                mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            } else {
                /* op is noncommutative and the order is not right */
                mpi_errno = MPIR_Sched_reduce(recvbuf, tmp_buf, count, datatype, op, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* copy result back into recvbuf */
                mpi_errno = MPIR_Sched_copy(tmp_buf, count, datatype, recvbuf, count, datatype, s);
                MPIR_ERR_CHECK(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            mask <<= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, rank - 1, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {        /* even */
            mpi_errno = MPIR_Sched_recv(recvbuf, count, datatype, rank + 1, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
