/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* helper callbacks and associated state structures */
struct shared_state {
    int sendcount;
    MPI_Aint curr_count;
    MPI_Aint send_subtree_count;
    int nbytes;
    MPI_Status status;
};
static int get_count(MPIR_Comm * comm, int tag, void *state)
{
    struct shared_state *ss = state;
    MPIR_Get_count_impl(&ss->status, MPI_BYTE, &ss->curr_count);
    return MPI_SUCCESS;
}

static int calc_send_count_root(MPIR_Comm * comm, int tag, void *state, void *state2)
{
    struct shared_state *ss = state;
    int mask = (int) (size_t) state2;
    ss->send_subtree_count = ss->curr_count - ss->sendcount * mask;
    return MPI_SUCCESS;
}

static int calc_send_count_non_root(MPIR_Comm * comm, int tag, void *state, void *state2)
{
    struct shared_state *ss = state;
    int mask = (int) (size_t) state2;
    ss->send_subtree_count = ss->curr_count - ss->nbytes * mask;
    return MPI_SUCCESS;
}

static int calc_curr_count(MPIR_Comm * comm, int tag, void *state)
{
    struct shared_state *ss = state;
    ss->curr_count -= ss->send_subtree_count;
    return MPI_SUCCESS;
}

/* any non-MPI functions go here, especially non-static ones */

/* This is the machine-independent implementation of scatter. The algorithm is:

   Algorithm: MPI_Scatter

   We use a binomial tree algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements:

   End Algorithm: MPI_Scatter
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_sched_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_sched_intra_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent = 0;
    int rank, comm_size, sendtype_size;
    int relative_rank;
    int mask, recvtype_size = 0, src, dst;
    int tmp_buf_size = 0;
    void *tmp_buf = NULL;
    struct shared_state *ss = NULL;
    MPIR_SCHED_CHKPMEM_DECL(4);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (((rank == root) && (sendcount == 0)) || ((rank != root) && (recvcount == 0)))
        goto fn_exit;

    MPIR_SCHED_CHKPMEM_MALLOC(ss, struct shared_state *, sizeof(struct shared_state), mpi_errno,
                              "shared_state", MPL_MEM_BUFFER);
    ss->sendcount = sendcount;

    if (rank == root)
        MPIR_Datatype_get_extent_macro(sendtype, extent);

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root) {
        /* We separate the two cases (root and non-root) because
         * in the event of recvbuf=MPI_IN_PLACE on the root,
         * recvcount and recvtype are not valid */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                         extent * sendcount * comm_size);

        ss->nbytes = sendtype_size * sendcount;
    } else {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        MPIR_Ensure_Aint_fits_in_pointer(extent * recvcount * comm_size);
        ss->nbytes = recvtype_size * recvcount;
    }

    ss->curr_count = 0;

    /* all even nodes other than root need a temporary buffer to
     * receive data of max size (ss->nbytes*comm_size)/2 */
    if (relative_rank && !(relative_rank % 2)) {
        tmp_buf_size = (ss->nbytes * comm_size) / 2;
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf",
                                  MPL_MEM_BUFFER);
    }

    /* if the root is not rank 0, we reorder the sendbuf in order of
     * relative ranks and copy it into a temporary buffer, so that
     * all the sends from the root are contiguous and in the right
     * order. */
    if (rank == root) {
        if (root != 0) {
            tmp_buf_size = ss->nbytes * comm_size;
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf",
                                      MPL_MEM_BUFFER);

            if (recvbuf != MPI_IN_PLACE)
                mpi_errno = MPIR_Sched_copy(((char *) sendbuf + extent * sendcount * rank),
                                            sendcount * (comm_size - rank), sendtype,
                                            tmp_buf, ss->nbytes * (comm_size - rank), MPI_BYTE, s);
            else
                mpi_errno =
                    MPIR_Sched_copy(((char *) sendbuf + extent * sendcount * (rank + 1)),
                                    sendcount * (comm_size - rank - 1), sendtype,
                                    ((char *) tmp_buf + ss->nbytes),
                                    ss->nbytes * (comm_size - rank - 1), MPI_BYTE, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            mpi_errno = MPIR_Sched_copy(sendbuf, sendcount * rank, sendtype,
                                        ((char *) tmp_buf + ss->nbytes * (comm_size - rank)),
                                        ss->nbytes * rank, MPI_BYTE, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            MPIR_SCHED_BARRIER(s);
            ss->curr_count = ss->nbytes * comm_size;
        } else
            ss->curr_count = sendcount * comm_size;
    }

    /* root has all the data; others have zero so far */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;

            /* The leaf nodes receive directly into recvbuf because
             * they don't have to forward data to anyone. Others
             * receive data into a temporary buffer. */
            if (relative_rank % 2) {
                mpi_errno = MPIR_Sched_recv(recvbuf, recvcount, recvtype, src, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            } else {

                /* the recv size is larger than what may be sent in
                 * some cases. query amount of data actually received */
                mpi_errno =
                    MPIR_Sched_recv_status(tmp_buf, tmp_buf_size, MPI_BYTE, src, comm_ptr,
                                           &ss->status, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                mpi_errno = MPIR_Sched_cb(&get_count, ss, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB upto (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down
     * one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size)
                dst -= comm_size;

            if ((rank == root) && (root == 0)) {
#if 0
                /* FIXME how can this be right? shouldn't (sendcount*mask)
                 * be the amount sent and curr_cnt be reduced by that?  Or
                 * is it always true the (curr_cnt/2==sendcount*mask)? */
                send_subtree_cnt = curr_cnt - sendcount * mask;
#endif
                mpi_errno = MPIR_Sched_cb2(&calc_send_count_root, ss, ((void *) (size_t) mask), s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* mask is also the size of this process's subtree */
                mpi_errno =
                    MPIR_Sched_send_defer(((char *) sendbuf + extent * sendcount * mask),
                                          &ss->send_subtree_count, sendtype, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            } else {
                /* non-zero root and others */
                mpi_errno =
                    MPIR_Sched_cb2(&calc_send_count_non_root, ss, ((void *) (size_t) mask), s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* mask is also the size of this process's subtree */
                mpi_errno = MPIR_Sched_send_defer(((char *) tmp_buf + ss->nbytes * mask),
                                                  &ss->send_subtree_count, MPI_BYTE, dst,
                                                  comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            mpi_errno = MPIR_Sched_cb(&calc_curr_count, ss, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        mask >>= 1;
    }

    if ((rank == root) && (root == 0) && (recvbuf != MPI_IN_PLACE)) {
        /* for root=0, put root's data in recvbuf if not MPI_IN_PLACE */
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    } else if (!(relative_rank % 2) && (recvbuf != MPI_IN_PLACE)) {
        /* for non-zero root and non-leaf nodes, copy from tmp_buf
         * into recvbuf */
        mpi_errno = MPIR_Sched_copy(tmp_buf, ss->nbytes, MPI_BYTE, recvbuf, recvcount, recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
