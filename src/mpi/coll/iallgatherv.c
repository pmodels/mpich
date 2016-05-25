/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iallgatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallgatherv = PMPI_Iallgatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallgatherv  MPI_Iallgatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallgatherv as PMPI_Iallgatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request)
                    __attribute__((weak,alias("PMPI_Iallgatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallgatherv
#define MPI_Iallgatherv PMPI_Iallgatherv

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_rec_dbl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_rec_dbl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const int recvcounts[], const int displs[],
                             MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, i, j, k;
    int curr_count, send_offset, incoming_count, recv_offset;
    int mask, dst, total_count, position, offset, my_tree_root, dst_tree_root;
    MPI_Aint recvtype_extent, recvtype_true_extent, recvtype_true_lb;
    void *tmp_buf = NULL;
    int is_homogeneous ATTRIBUTE((unused));
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_Assert(is_homogeneous); /* we only handle the homogeneous for now */

    /* need to receive contiguously into tmp_buf because
       displs could make the recvbuf noncontiguous */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    MPIR_Ensure_Aint_fits_in_pointer(total_count*(MPL_MAX(recvtype_true_extent, recvtype_extent)));
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, total_count*(MPL_MAX(recvtype_true_extent,recvtype_extent)), mpi_errno, "tmp_buf");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - recvtype_true_lb);

    /* copy local data into right location in tmp_buf */
    position = 0;
    for (i=0; i<rank; i++) position += recvcounts[i];
    if (sendbuf != MPI_IN_PLACE)
    {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                   ((char *)tmp_buf + position*recvtype_extent),
                                   recvcounts[rank], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else
    {
        /* if in_place specified, local data is found in recvbuf */
        mpi_errno = MPIR_Sched_copy(((char *)recvbuf + displs[rank]*recvtype_extent),
                                   recvcounts[rank], recvtype,
                                   ((char *)tmp_buf + position*recvtype_extent),
                                   recvcounts[rank], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    curr_count = recvcounts[rank];

    /* never used uninitialized w/o this, but compiler can't tell that */
    incoming_count = -1;

    /* [goodell@] random notes that help slightly when deciphering this code:
     * - mask is also equal to the number of blocks that we are going to recv
     *   (less if comm_size is non-pof2)
     * - FOO_tree_root is the leftmost (lowest ranked) process with whom FOO has
     *   communicated, directly or indirectly, at the beginning of round the
     *   round.  FOO is either "dst" or "my", where "my" means use my rank.
     * - in each round we are going to recv the blocks
     *   B[dst_tree_root],B[dst_tree_root+1],...,B[min(dst_tree_root+mask,comm_size)]
     */
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

        if (dst < comm_size) {
            send_offset = 0;
            for (j = 0; j < my_tree_root; j++)
                send_offset += recvcounts[j];

            recv_offset = 0;
            for (j = 0; j < dst_tree_root; j++)
                recv_offset += recvcounts[j];

            incoming_count = 0;
            for (j = dst_tree_root; j < (dst_tree_root + mask) && j < comm_size; ++j)
                incoming_count += recvcounts[j];

            mpi_errno = MPIR_Sched_send(((char *)tmp_buf + send_offset * recvtype_extent),
                                        curr_count, recvtype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + recv_offset * recvtype_extent),
                                        incoming_count, recvtype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            curr_count += incoming_count;
        }

        /* if some processes in this process's subtree in this step
           did not have any destination process to communicate with
           because of non-power-of-two, we need to send them the
           data that they would normally have received from those
           processes. That is, the haves in this subtree must send to
           the havenots. We use a logarithmic
           recursive-halfing algorithm for this. */

        /* This part of the code will not currently be
           executed because we are not using recursive
           doubling for non power of two. Mark it as experimental
           so that it doesn't show up as red in the coverage
           tests. */

        /* --BEGIN EXPERIMENTAL-- */
        if (dst_tree_root + mask > comm_size) {
            int tmp_mask, tree_root;
            int nprocs_completed = comm_size - my_tree_root - mask;
            /* nprocs_completed is the number of processes in this
               subtree that have all the data. Send data to others
               in a tree fashion. First find root of current tree
               that is being divided into two. k is the number of
               least-significant bits in this process's rank that
               must be zeroed out to find the rank of the root */
            /* [goodell@] it looks like (k==i) is always true, could possibly
             * skip the loop below */
            j = mask;
            k = 0;
            while (j) {
                j >>= 1;
                k++;
            }
            k--;

            tmp_mask = mask >> 1;

            while (tmp_mask) {
                dst = rank ^ tmp_mask;

                tree_root = rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                   doesn't have data. at any step, multiple processes
                   can send if they have the data */
                if ((dst > rank) &&
                    (rank < tree_root + nprocs_completed) &&
                    (dst >= tree_root + nprocs_completed))
                {
                    offset = 0;
                    for (j = 0; j < (my_tree_root+mask); j++)
                        offset += recvcounts[j];
                    offset *= recvtype_extent;

                    /* incoming_count was set in the previous
                       receive. that's the amount of data to be
                       sent now. */
                    mpi_errno = MPIR_Sched_send(((char *)tmp_buf + offset),
                                                incoming_count, recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                /* recv only if this proc. doesn't have data and sender
                   has data */
                else if ((dst < rank) &&
                         (dst < tree_root + nprocs_completed) &&
                         (rank >= tree_root + nprocs_completed))
                {

                    offset = 0;
                    for (j = 0; j < (my_tree_root+mask); j++)
                        offset += recvcounts[j];

                    /* recalculate incoming_count, since not all processes will have
                     * this value */
                    incoming_count = 0;
                    for (j = dst_tree_root; j < (dst_tree_root + mask) && j < comm_size; ++j)
                        incoming_count += recvcounts[j];

                    mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + offset * recvtype_extent),
                                                incoming_count, recvtype,
                                                dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                    curr_count += incoming_count;
                }
                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }

    /* sanity check that we got all of the data blocks */
    MPIR_Assert(curr_count == total_count);

    /* copy data from tmp_buf to recvbuf */
    position = 0;
    for (j = 0; j < comm_size; j++) {
        if ((sendbuf != MPI_IN_PLACE) || (j != rank)) {
            /* not necessary to copy if in_place and
               j==rank. otherwise copy. */
            mpi_errno = MPIR_Sched_copy(((char *)tmp_buf + position*recvtype_extent),
                                       recvcounts[j], recvtype,
                                       ((char *)recvbuf + displs[j]*recvtype_extent),
                                       recvcounts[j], recvtype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        position += recvcounts[j];
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_bruck
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_bruck(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[], const int displs[],
                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
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
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    /* allocate a temporary buffer of the same size as recvbuf. */

    /* get true extent of recvtype */
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(recvtype_true_extent, recvtype_extent));
    recvbuf_extent = total_count * (MPL_MAX(recvtype_true_extent, recvtype_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, recvbuf_extent, mpi_errno, "tmp_buf");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - recvtype_true_lb);

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    tmp_buf, recvcounts[rank], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }
    else {
        mpi_errno = MPIR_Sched_copy(((char *)recvbuf + displs[rank]*recvtype_extent),
                                    recvcounts[rank], recvtype,
                                    tmp_buf, recvcounts[rank], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
    while (pof2 <= comm_size/2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        incoming_count = 0;
        for (i = 0; i < pof2; ++i) {
            incoming_count += recvcounts[(src + i) % comm_size];
        }

        mpi_errno = MPIR_Sched_send(tmp_buf, curr_count, recvtype, dst, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier */
        mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + curr_count*recvtype_extent),
                                    incoming_count, recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
        for (i=0; i<rem; i++)
            send_cnt += recvcounts[(rank+i)%comm_size];

        mpi_errno = MPIR_Sched_send(tmp_buf, send_cnt, recvtype, dst, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier */
        mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + curr_count*recvtype_extent),
                                     (total_count - curr_count), recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    send_cnt = 0;
    for (i=0; i < (comm_size-rank); i++) {
        j = (rank+i)%comm_size;
        mpi_errno = MPIR_Sched_copy(((char *)tmp_buf + send_cnt*recvtype_extent),
                                    recvcounts[j], recvtype,
                                    ((char *)recvbuf + displs[j]*recvtype_extent),
                                    recvcounts[j], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        send_cnt += recvcounts[j];
    }

    for (i=0; i<rank; i++) {
        mpi_errno = MPIR_Sched_copy(((char *)tmp_buf + send_cnt*recvtype_extent),
                                    recvcounts[i], recvtype,
                                    ((char *)recvbuf + displs[i]*recvtype_extent),
                                    recvcounts[i], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        send_cnt += recvcounts[i];
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int recvcounts[], const int displs[],
                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, total_count;
    MPI_Aint recvtype_extent;
    int rank, comm_size;
    int left, right;
    char *sbuf = NULL;
    char *rbuf = NULL;
    int soffset, roffset;
    int torecv, tosend, min;
    int sendnow, recvnow;
    int sidx, ridx;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    if (sendbuf != MPI_IN_PLACE) {
        /* First, load the "local" version in the recvbuf. */
        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                    ((char *)recvbuf + displs[rank]*recvtype_extent),
                                    recvcounts[rank], recvtype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    left  = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    torecv = total_count - recvcounts[rank];
    tosend = total_count - recvcounts[right];

    min = recvcounts[0];
    for (i = 1; i < comm_size; i++)
        if (min > recvcounts[i])
            min = recvcounts[i];
    if (min * recvtype_extent < MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE)
        min = MPIR_CVAR_ALLGATHERV_PIPELINE_MSG_SIZE / recvtype_extent;
    /* Handle the case where the datatype extent is larger than
     * the pipeline size. */
    if (!min)
        min = 1;

    sidx = rank;
    ridx = left;
    soffset = 0;
    roffset = 0;
    while (tosend || torecv) { /* While we have data to send or receive */
        sendnow = ((recvcounts[sidx] - soffset) > min) ? min : (recvcounts[sidx] - soffset);
        recvnow = ((recvcounts[ridx] - roffset) > min) ? min : (recvcounts[ridx] - roffset);
        sbuf = (char *)recvbuf + ((displs[sidx] + soffset) * recvtype_extent);
        rbuf = (char *)recvbuf + ((displs[ridx] + roffset) * recvtype_extent);

        /* Protect against wrap-around of indices */
        if (!tosend)
            sendnow = 0;
        if (!torecv)
            recvnow = 0;

        /* Communicate */
        if (recvnow) { /* If there's no data to send, just do a recv call */
            mpi_errno = MPIR_Sched_recv(rbuf, recvnow, recvtype, left, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            torecv -= recvnow;
        }
        if (sendnow) { /* If there's no data to receive, just do a send call */
            mpi_errno = MPIR_Sched_send(sbuf, sendnow, recvtype, right, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            tosend -= sendnow;
        }
        MPIR_SCHED_BARRIER(s);

        soffset += sendnow;
        roffset += recvnow;
        if (soffset == recvcounts[sidx]) {
            soffset = 0;
            sidx = (sidx + comm_size - 1) % comm_size;
        }
        if (roffset == recvcounts[ridx]) {
            roffset = 0;
            ridx = (ridx + comm_size - 1) % comm_size;
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of allgatherv. The algorithm is:

   Algorithm: MPI_Allgatherv

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

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements:

   End Algorithm: MPI_Allgatherv
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[], const int displs[],
                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size, total_count, recvtype_size;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    if ((total_count*recvtype_size < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) &&
        !(comm_size & (comm_size - 1)))
    {
        /* Short or medium size message and power-of-two no. of processes. Use
         * recursive doubling algorithm */
        mpi_errno = MPIR_Iallgatherv_rec_dbl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else if (total_count*recvtype_size < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        /* Short message and non-power-of-two no. of processes. Use
         * Bruck algorithm (see description above). */
        mpi_errno = MPIR_Iallgatherv_bruck(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* long message or medium-size message and non-power-of-two
         * no. of processes. Use ring algorithm. */
        mpi_errno = MPIR_Iallgatherv_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[], const int displs[],
                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
/* Intercommunicator Allgatherv.
   This is done differently from the intercommunicator allgather
   because we don't have all the information to do a local
   intracommunictor gather (sendcount can be different on each
   process). Therefore, we do the following:
   Each group first does an intercommunicator gather to rank 0
   and then does an intracommunicator broadcast.
*/
    int mpi_errno = MPI_SUCCESS;
    int remote_size, root, rank;
    MPIR_Comm *newcomm_ptr = NULL;
    MPI_Datatype newtype = MPI_DATATYPE_NULL;

    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    MPIR_Assert(comm_ptr->coll_fns && comm_ptr->coll_fns->Igatherv_sched);

    /* first do an intercommunicator gatherv from left to right group,
       then from right to left group */
    if (comm_ptr->is_low_group) {
        /* gatherv from right group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = comm_ptr->coll_fns->Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                 recvcounts, displs, recvtype, root,
                                                 comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* gatherv to right group */
        root = 0;
        mpi_errno = comm_ptr->coll_fns->Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                 recvcounts, displs, recvtype, root,
                                                 comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* gatherv to left group  */
        root = 0;
        mpi_errno = comm_ptr->coll_fns->Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                 recvcounts, displs, recvtype, root,
                                                 comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* gatherv from left group */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = comm_ptr->coll_fns->Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf,
                                                 recvcounts, displs, recvtype, root,
                                                 comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

    /* now do an intracommunicator broadcast within each group. we use
       a derived datatype to handle the displacements */

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        mpi_errno = MPII_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;
    MPIR_Assert(newcomm_ptr->coll_fns && newcomm_ptr->coll_fns->Ibcast_sched);

    mpi_errno = MPIR_Type_indexed_impl(remote_size, recvcounts, displs, recvtype, &newtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Type_commit_impl(&newtype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = newcomm_ptr->coll_fns->Ibcast_sched(recvbuf, 1, newtype, 0, newcomm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Type_free_impl(&newtype);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int recvcounts[], const int displs[],
                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request)
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
    MPIR_Assert(comm_ptr->coll_fns->Iallgatherv_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iallgatherv_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
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
#define FUNCNAME MPI_Iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iallgatherv - Gathers data from all tasks and deliver the combined data
                  to all tasks in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) containing the number of elements that are received from each process
. displs - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLGATHERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLGATHERV);

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
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPIR_Datatype *sendtype_ptr = NULL;
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                }

                /* catch common aliasing cases */
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                        sendtype == recvtype &&
                        recvcounts[comm_ptr->rank] != 0 &&
                        sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + displs[comm_ptr->rank]*recvtype_size, mpi_errno);
                }
            }

            MPIR_ERRTEST_ARGNULL(recvcounts,"recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(displs,"displs", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLGATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iallgatherv", "**mpi_iallgatherv %p %d %D %p %p %p %D %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
