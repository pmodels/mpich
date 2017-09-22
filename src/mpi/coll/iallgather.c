/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iallgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallgather = PMPI_Iallgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallgather  MPI_Iallgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallgather as PMPI_Iallgather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
                   __attribute__((weak,alias("PMPI_Iallgather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallgather
#define MPI_Iallgather PMPI_Iallgather

/* any non-MPI functions go here, especially non-static ones */

/* helper callbacks and associated state structures */
struct shared_state {
    int recvtype;
    MPI_Aint curr_count;
    MPI_Aint last_recv_count;
    MPI_Status status;
};
static int get_count(MPIR_Comm *comm, int tag, void *state)
{
    struct shared_state *ss = state;
    int recv_count;
    MPIR_Get_count_impl(&ss->status, ss->recvtype, &recv_count);
    ss->last_recv_count = recv_count;
    ss->curr_count += ss->last_recv_count;
    return MPI_SUCCESS;
}
static int dtp_release_ref(MPIR_Comm *comm, int tag, void *state)
{
    MPIR_Datatype *recv_dtp = state;
    MPIR_Datatype_release(recv_dtp);
    return MPI_SUCCESS;
}

/* allgather implemented by recursive doubling, only for power-of-2 processes */
/* TODO does not currently handle the heterogeneous case */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_rec_dbl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_rec_dbl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    struct shared_state *ss = NULL;
    int comm_size, rank;
    int i, j, k;
    int mask, tmp_mask, dst;
    int dst_tree_root, my_tree_root, tree_root;
    int offset, send_offset, recv_offset;
    MPI_Aint recvtype_extent;
    MPIR_Datatype *recv_dtp;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    recv_dtp = NULL;
    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
        MPIR_Datatype_get_ptr(recvtype, recv_dtp);
    }

    MPIR_Datatype_get_extent_macro( recvtype, recvtype_extent );

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     (comm_size * recvcount * recvtype_extent));

    /*  copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    ((char *)recvbuf + rank*recvcount*recvtype_extent),
                                    recvcount, recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_CHKPMEM_MALLOC(ss, struct shared_state *, sizeof(struct shared_state), mpi_errno, "ss");
    ss->curr_count = recvcount;
    ss->recvtype = recvtype;
    /* ensure that recvtype doesn't disappear immediately after last _recv but before _cb */
    if (recv_dtp)
        MPIR_Datatype_add_ref(recv_dtp);

    mask = 0x1;
    i = 0;
    while (mask < comm_size) {
        dst = rank ^ mask;

        /* find offset into send and recv buffers. zero out
           the least significant "i" bits of rank and dst to
           find root of src and dst subtrees. Use ranks of
           roots as index to send from and recv into buffer */

        dst_tree_root = dst >> i;
        dst_tree_root <<= i;

        my_tree_root = rank >> i;
        my_tree_root <<= i;

        /* saving an MPI_Aint into an int, overflow checked above */
        send_offset = my_tree_root * recvcount * recvtype_extent;
        recv_offset = dst_tree_root * recvcount * recvtype_extent;

        if (dst < comm_size) {
            mpi_errno = MPIR_Sched_send_defer(((char *)recvbuf + send_offset),
                                              &ss->curr_count, recvtype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* send-recv, no sched barrier here */
            mpi_errno = MPIR_Sched_recv_status(((char *)recvbuf + recv_offset),
                                               ((comm_size-dst_tree_root)*recvcount),
                                               recvtype, dst, comm_ptr, &ss->status, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            mpi_errno = MPIR_Sched_cb(&get_count, ss, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

        /* if some processes in this process's subtree in this step
           did not have any destination process to communicate with
           because of non-power-of-two, we need to send them the
           data that they would normally have received from those
           processes. That is, the haves in this subtree must send to
           the havenots. We use a logarithmic recursive-halfing algorithm
           for this. */

        /* This part of the code will not currently be
           executed because we are not using recursive
           doubling for non power of two. Mark it as experimental
           so that it doesn't show up as red in the coverage
           tests. */
        /* --BEGIN EXPERIMENTAL-- */
        if (dst_tree_root + mask > comm_size) {
            int nprocs_completed = comm_size - my_tree_root - mask;
            /* nprocs_completed is the number of processes in this
               subtree that have all the data. Send data to others
               in a tree fashion. First find root of current tree
               that is being divided into two. k is the number of
               least-significant bits in this process's rank that
               must be zeroed out to find the rank of the root */
            j = mask;
            k = 0;
            while (j) {
                j >>= 1;
                k++;
            }
            k--;

            /* FIXME: saving an MPI_Aint into an int */
            offset = recvcount * (my_tree_root + mask) * recvtype_extent;
            tmp_mask = mask >> 1;

            while (tmp_mask) {
                dst = rank ^ tmp_mask;

                tree_root = rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                   doesn't have data. at any step, multiple processes
                   can send if they have the data */
                if ((dst > rank) &&
                    (rank < tree_root + nprocs_completed)
                    && (dst >= tree_root + nprocs_completed))
                {
                    /* last_recv_count was set in the previous
                       receive. that's the amount of data to be
                       sent now. */
                    mpi_errno = MPIR_Sched_send_defer(((char *)recvbuf + offset),
                                                      &ss->last_recv_count,
                                                      recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                /* recv only if this proc. doesn't have data and sender
                   has data */
                else if ((dst < rank) &&
                         (dst < tree_root + nprocs_completed) &&
                         (rank >= tree_root + nprocs_completed))
                {
                    /* nprocs_completed is also equal to the
                       no. of processes whose data we don't have */
                    mpi_errno = MPIR_Sched_recv_status(((char *)recvbuf + offset),
                                                       ((comm_size - (my_tree_root + mask))*recvcount),
                                                       recvtype, dst, comm_ptr, &ss->status, s);
                    MPIR_SCHED_BARRIER(s);
                    mpi_errno = MPIR_Sched_cb(&get_count, ss, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }

                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }

    if (recv_dtp) {
        mpi_errno = MPIR_Sched_cb(dtp_release_ref, recv_dtp, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_bruck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
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

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void*, recvbuf_extent, mpi_errno, "tmp_buf");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - recvtype_true_lb);

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    tmp_buf, recvcount, recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }
    else {
        mpi_errno = MPIR_Sched_copy(((char *)recvbuf + rank * recvcount * recvtype_extent),
                                    recvcount, recvtype, tmp_buf,
                                    recvcount, recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* do the first \floor(\lg p) steps */

    curr_cnt = recvcount;
    pof2 = 1;
    while (pof2 <= comm_size/2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        mpi_errno = MPIR_Sched_send(tmp_buf, curr_cnt, recvtype, dst, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* logically sendrecv, so no barrier here */
        mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + curr_cnt*recvtype_extent),
                                    curr_cnt, recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* logically sendrecv, so no barrier here */
        mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + curr_cnt*recvtype_extent),
                                    (rem * recvcount), recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    mpi_errno = MPIR_Sched_copy(tmp_buf, ((comm_size-rank)*recvcount), recvtype,
                                ((char *) recvbuf + rank*recvcount*recvtype_extent),
                                ((comm_size-rank)*recvcount), recvtype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    if (rank) {
        mpi_errno = MPIR_Sched_copy(((char *) tmp_buf + (comm_size-rank)*recvcount*recvtype_extent),
                                   rank*recvcount, recvtype,
                                   recvbuf, rank*recvcount, recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;
    int i, j, jnext, left, right;
    MPI_Aint recvtype_extent;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     (comm_size * recvcount * recvtype_extent));

    /* First, load the "local" version in the recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    ((char *)recvbuf + rank*recvcount*recvtype_extent),
                                    recvcount, recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* Now, send left to right.  This fills in the receive area in
     * reverse order. */
    left  = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    j     = rank;
    jnext = left;
    for (i=1; i<comm_size; i++) {
        mpi_errno = MPIR_Sched_send(((char *)recvbuf + j*recvcount*recvtype_extent),
                                     recvcount, recvtype, right, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* concurrent, no barrier here */
        mpi_errno = MPIR_Sched_recv(((char *)recvbuf + jnext*recvcount*recvtype_extent),
                                     recvcount, recvtype, left, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        j = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This is the default implementation of allgather. The algorithm is:

   Algorithm: MPI_Allgather

   For short messages and non-power-of-two no. of processes, we use
   the algorithm from the Jehoshua Bruck et al IEEE TPDS Nov 97
   paper. It is a variant of the disemmination algorithm for
   barrier. It takes ceiling(lg p) steps.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.

   For short or medium-size messages and power-of-two no. of
   processes, we use the recursive doubling algorithm.

   Cost = lgp.alpha + n.((p-1)/p).beta

   TODO: On TCP, we may want to use recursive doubling instead of the Bruck
   algorithm in all cases because of the pairwise-exchange property of
   recursive doubling (see Benson et al paper in Euro PVM/MPI
   2003).

   It is interesting to note that either of the above algorithms for
   MPI_Allgather has the same cost as the tree algorithm for MPI_Gather!

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   We use this algorithm instead of recursive doubling for long
   messages because we find that this communication pattern (nearest
   neighbor) performs twice as fast as recursive doubling for long
   messages (on Myrinet and IBM SP).

   Possible improvements:

   End Algorithm: MPI_Allgather
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, recvtype_size;
    int tot_bytes;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    tot_bytes = (MPI_Aint)recvcount * comm_size * recvtype_size;

    if ((tot_bytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        /* Short or medium size message and power-of-two no. of processes. Use
         * recursive doubling algorithm */
        mpi_errno = MPIR_Iallgather_rec_dbl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else if (tot_bytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        /* Short message and non-power-of-two no. of processes. Use
         * Bruck algorithm (see description above). */
        mpi_errno = MPIR_Iallgather_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* long message or medium-size message and non-power-of-two no. of
         * processes. use ring algorithm. */
        mpi_errno = MPIR_Iallgather_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    /* Intercommunicator Allgather.
       Each group does a gather to local root with the local
       intracommunicator, and then does an intercommunicator broadcast.
    */
    int mpi_errno = MPI_SUCCESS;
    int rank, local_size, remote_size, root;
    MPI_Aint true_extent, true_lb = 0, extent, send_extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    if ((rank == 0) && (sendcount != 0)) {
        /* In each group, rank 0 allocates temp. buffer for local
           gather */
        MPIR_Type_get_true_extent_impl(sendtype, &true_lb, &true_extent);

        MPIR_Datatype_get_extent_macro( sendtype, send_extent );
        extent = MPL_MAX(send_extent, true_extent);

        MPIR_Ensure_Aint_fits_in_pointer(extent * sendcount * local_size);
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void*, extent*sendcount*local_size, mpi_errno, "tmp_buf");

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        MPIR_Assert(newcomm_ptr->coll_fns && newcomm_ptr->coll_fns->Igather_sched);
        mpi_errno = newcomm_ptr->coll_fns->Igather_sched(sendbuf, sendcount, sendtype,
                                                   tmp_buf, sendcount, sendtype,
                                                   0, newcomm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* first broadcast from left to right group, then from right to
       left group */
    MPIR_Assert(comm_ptr->coll_fns && comm_ptr->coll_fns->Ibcast_sched);
    if (comm_ptr->is_low_group) {
        /* bcast to right*/
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = comm_ptr->coll_fns->Ibcast_sched(tmp_buf, sendcount*local_size,
                                                   sendtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = comm_ptr->coll_fns->Ibcast_sched(recvbuf, recvcount*remote_size,
                                                   recvtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }
    else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = comm_ptr->coll_fns->Ibcast_sched(recvbuf, recvcount*remote_size,
                                                   recvtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        /* no sched barrier, bcasts are logically concurrent */

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = comm_ptr->coll_fns->Ibcast_sched(tmp_buf, sendcount*local_size,
                                                   sendtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *reqp = NULL;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(comm_ptr->coll_fns != NULL);
    MPIR_Assert(comm_ptr->coll_fns->Iallgather_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iallgather_sched(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, &reqp);
    if (reqp)
        *request = reqp->handle;
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iallgather - Gathers data from all tasks and distribute the combined data
                 to all tasks in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements in receive buffer (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLGATHER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLGATHER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
            }
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (sendbuf != MPI_IN_PLACE && HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *sendtype_ptr = NULL;
                MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            /* catch common aliasing cases */
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcount == recvcount && sendcount != 0) {
                int recvtype_size;
                MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + comm_ptr->rank*recvcount*recvtype_size, mpi_errno);
            }

            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iallgather", "**mpi_iallgather %p %d %D %p %d %D %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
