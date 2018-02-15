/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_sched_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_sched_intra_binomial(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, is_commutative;
    int mask, relrank, source, lroot;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    MPIR_SCHED_CHKPMEM_DECL(2);

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (count == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    /* Create a temporary buffer */

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    is_commutative = MPIR_Op_is_commutative(op);

    /* I think this is the worse case, so we can avoid an assert()
     * inside the for loop */
    /* should be buf+{this}? */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                              mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* If I'm not the root, then my recvbuf may not be valid, therefore
     * I have to allocate a temporary one */
    if (rank != root) {
        MPIR_SCHED_CHKPMEM_MALLOC(recvbuf, void *,
                                  count * (MPL_MAX(extent, true_extent)),
                                  mpi_errno, "receive buffer", MPL_MEM_BUFFER);
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        /* could do this up front as an MPIR_Localcopy instead, but we'll defer
         * it to the progress engine */
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* This code is from MPICH-1. */

    /* Here's the algorithm.  Relative to the root, look at the bit pattern in
     * my rank.  Starting from the right (lsb), if the bit is 1, send to
     * the node with that bit zero and exit; if the bit is 0, receive from the
     * node with that bit set and combine (as long as that node is within the
     * group)
     *
     * Note that by receiving with source selection, we guarentee that we get
     * the same bits with the same input.  If we allowed the parent to receive
     * the children in any order, then timing differences could cause different
     * results (roundoff error, over/underflows in some cases, etc).
     *
     * Because of the way these are ordered, if root is 0, then this is correct
     * for both commutative and non-commutitive operations.  If root is not
     * 0, then for non-commutitive, we use a root of zero and then send
     * the result to the root.  To see this, note that the ordering is
     * mask = 1: (ab)(cd)(ef)(gh)            (odds send to evens)
     * mask = 2: ((ab)(cd))((ef)(gh))        (3,6 send to 0,4)
     * mask = 4: (((ab)(cd))((ef)(gh)))      (4 sends to 0)
     *
     * Comments on buffering.
     * If the datatype is not contiguous, we still need to pass contiguous
     * data to the user routine.
     * In this case, we should make a copy of the data in some format,
     * and send/operate on that.
     *
     * In general, we can't use MPI_PACK, because the alignment of that
     * is rather vague, and the data may not be re-usable.  What we actually
     * need is a "squeeze" operation that removes the skips.
     */
    mask = 0x1;
    if (is_commutative)
        lroot = root;
    else
        lroot = 0;
    relrank = (rank - lroot + comm_size) % comm_size;

    while (mask < comm_size) {
        /* Receive */
        if ((mask & relrank) == 0) {
            source = (relrank | mask);
            if (source < comm_size) {
                source = (source + lroot) % comm_size;
                mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, source, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_barrier(s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

                /* The sender is above us, so the received buffer must be
                 * the second argument (in the noncommutative case). */
                if (is_commutative) {
                    mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                } else {
                    mpi_errno = MPIR_Sched_reduce(recvbuf, tmp_buf, count, datatype, op, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_barrier(s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    mpi_errno =
                        MPIR_Sched_copy(tmp_buf, count, datatype, recvbuf, count, datatype, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
                mpi_errno = MPIR_Sched_barrier(s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
        } else {
            /* I've received all that I'm going to.  Send my result to
             * my parent */
            source = ((relrank & (~mask)) + lroot) % comm_size;
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, source, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            break;
        }
        mask <<= 1;
    }

    if (!is_commutative && (root != 0)) {
        if (rank == 0) {
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, root, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        } else if (rank == root) {
            mpi_errno = MPIR_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
