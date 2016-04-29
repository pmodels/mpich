/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ibcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibcast = PMPI_Ibcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibcast  MPI_Ibcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibcast as PMPI_Ibcast
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request *request) __attribute__((weak,alias("PMPI_Ibcast")));
#endif
/* -- End Profiling Symbol Block */

struct MPIR_Ibcast_status{
    int curr_bytes;
    int n_bytes;
    MPI_Status status;
};
/* Add some functions for asynchronous error detection */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibcast
#define MPI_Ibcast PMPI_Ibcast

#undef FUNCNAME
#define FUNCNAME sched_test_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

static int sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    int recv_size;
    struct  MPIR_Ibcast_status *status = (struct MPIR_Ibcast_status*) state;
    MPIR_Get_count_impl(&status->status, MPI_BYTE, &recv_size);
    if(status->n_bytes != recv_size || status->status.MPI_ERROR != MPI_SUCCESS) {
         mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE,
               FCNAME, __LINE__, MPI_ERR_OTHER,
                     "**collective_size_mismatch",
		          "**collective_size_mismatch %d %d", status->n_bytes, recv_size);
    }
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME sched_test_curr_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

static int sched_test_curr_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct  MPIR_Ibcast_status *status = (struct MPIR_Ibcast_status*) state;
    if(status->n_bytes != status->curr_bytes) {
        mpi_errno = MPIR_Err_create_code( mpi_errno, MPIR_ERR_RECOVERABLE,
               FCNAME, __LINE__, MPI_ERR_OTHER,
                     "**collective_size_mismatch",
		          "**collective_size_mismatch %d %d", status->n_bytes, status->curr_bytes);
    }
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME sched_add_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

static int sched_add_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    int recv_size;
    struct  MPIR_Ibcast_status *status = (struct MPIR_Ibcast_status*) state;
    MPIR_Get_count_impl(&status->status, MPI_BYTE, &recv_size);
    status->curr_bytes+= recv_size;
    return mpi_errno;
}

/* any non-MPI functions go here, especially non-static ones */

/* A binomial tree broadcast algorithm.  Good for short messages,
   Cost = lgp.alpha + n.lgp.beta */
/* Adds operations to the given schedule that correspond to the specified
 * binomial broadcast.  It does _not_ start the schedule.  This permits callers
 * to build up a larger hierarchical broadcast from multiple invocations of this
 * function. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int mask;
    int comm_size, rank;
    int is_contig, is_homogeneous;
    MPI_Aint nbytes, type_size;
    int relative_rank;
    int src, dst;
    struct MPIR_Ibcast_status *status;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (comm_size == 1) {
        /* nothing to add, this is a useless broadcast */
        goto fn_exit;
    }

    MPIR_Datatype_is_contig(datatype, &is_contig);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_SCHED_CHKPMEM_MALLOC(status, struct MPIR_Ibcast_status *,
                              sizeof(struct MPIR_Ibcast_status), mpi_errno, "MPI_Stauts");


    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPIR_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

    nbytes = type_size * count;

    status->n_bytes = nbytes;

    if (!is_contig || !is_homogeneous)
    {
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_PACKED, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Use short message algorithm, namely, binomial tree */

    /* Algorithm:
       This uses a fairly basic recursive subdivision algorithm.
       The root sends to the process comm_size/2 away; the receiver becomes
       a root for a subtree and applies the same process. 

       So that the new root can easily identify the size of its
       subtree, the (subtree) roots are all powers of two (relative
       to the root) If m = the first power of 2 such that 2^m >= the
       size of the communicator, then the subtree at root at 2^(m-k)
       has size 2^k (with special handling for subtrees that aren't
       a power of two in size).

       Do subdivision.  There are two phases:
       1. Wait for arrival of data.  Because of the power of two nature
       of the subtree roots, the source of this message is always the
       process whose relative rank has the least significant 1 bit CLEARED.
       That is, process 4 (100) receives from process 0, process 7 (111)
       from process 6 (110), etc.
       2. Forward to my subtree

       Note that the process that is the tree root is handled automatically
       by this code, since it has no bits set.  */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask; 
            if (src < 0) src += comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPIR_Sched_recv_status(tmp_buf, nbytes, MPI_BYTE, src,
                                                    comm_ptr, &status->status, s);
            else
                mpi_errno = MPIR_Sched_recv_status(buffer, count, datatype, src,
                                                   comm_ptr, &status->status, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            MPIR_SCHED_BARRIER(s);
            if(is_homogeneous){
                mpi_errno = MPIR_Sched_cb(&sched_test_length, status, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
       set from the LSB upto (but not including) mask.  Because of
       the "not including", we start by shifting mask back down one.

       We can easily change to a different algorithm at any power of two
       by changing the test (mask > 1) to (mask > block_size) 

       One such version would use non-blocking operations for the last 2-4
       steps (this also bounds the number of MPI_Requests that would
       be needed).  */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size) dst -= comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPIR_Sched_send(tmp_buf, nbytes, MPI_BYTE, dst, comm_ptr, s);
            else
                mpi_errno = MPIR_Sched_send(buffer, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* NOTE: This is departure from MPIR_Bcast_binomial.  A true analog
             * would put an MPIR_Sched_barrier here after every send. */
        }
        mask >>= 1;
    }

    if (!is_contig || !is_homogeneous) {
        if (rank != root) {
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_PACKED, buffer, count, datatype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

/* TODO it would be nice if we could refactor things to minimize
   duplication between this and MPIR_Iscatter_intra and friends.  We can't use
   MPIR_Iscatter_intra as is without inducing an extra copy in the noncontig case. */
/* This is a binomial scatter operation, but it does *not* take typical scatter
 * arguments.  At the moment this function always scatters a buffer of nbytes
 * starting at tmp_buf address. */
#undef FUNCNAME
#define FUNCNAME MPIR_Iscatter_for_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatter_for_bcast(void *tmp_buf, int root, MPIR_Comm *comm_ptr, int nbytes, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, src, dst;
    int relative_rank, mask;
    int scatter_size, curr_size, recv_size, send_size;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* The scatter algorithm divides the buffer into nprocs pieces and
       scatters them among the processes. Root gets the first piece,
       root+1 gets the second piece, and so forth. Uses the same binomial
       tree algorithm as above. Ceiling division
       is used to compute the size of each piece. This means some
       processes may not get any data. For example if bufsize = 97 and
       nprocs = 16, ranks 15 and 16 will get 0 data. On each process, the
       scattered data is stored at the same offset in the buffer as it is
       on the root process. */

    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0; /* root starts with all the data */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;

            /* compute the exact recv_size to avoid writing this NBC in callback style*/
            recv_size = nbytes - (relative_rank * scatter_size);
            if (recv_size < 0)
                recv_size = 0;

            curr_size = recv_size;

            if (recv_size > 0) {
                mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + relative_rank*scatter_size),
                                            recv_size, MPI_BYTE, src, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
       set from the LSB upto (but not including) mask.  Because of
       the "not including", we start by shifting mask back down
       one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            send_size = curr_size - scatter_size * mask;
            /* mask is also the size of this process's subtree */

            if (send_size > 0) {
                dst = rank + mask;
                if (dst >= comm_size)
                    dst -= comm_size;
                mpi_errno = MPIR_Sched_send(((char *)tmp_buf + scatter_size*(relative_rank+mask)),
                                            send_size, MPI_BYTE, dst, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                curr_size -= send_size;
            }
        }
        mask >>= 1;
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/*
   Broadcast based on a scatter followed by an allgather.

   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   For the allgather, we use a recursive doubling algorithm for
   medium-size messages and power-of-two number of processes. This
   takes lgp steps. In each step pairs of processes exchange all the
   data they have (we take care of non-power-of-two situations). This
   costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
   because it may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) Therefore, for long messages
   Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta
*/
/* It would be nice to just call:
 * ----8<----
 * comm_ptr->coll_fns->Iscatter_sched(...);
 * comm_ptr->coll_fns->Iallgather_sched(...);
 * ----8<----
 *
 * But that results in inefficient additional memory allocation and copies
 * because the scatter doesn't know that we have the whole bcast buffer to use
 * as a temp buffer for forwarding.  Furthermore, there's no guarantee that
 * nbytes is a multiple of comm_size, and the regular scatter/allgather ops
 * can't cope with that case correctly.  We could use scatterv/allgatherv
 * instead, but that's not scalable in comm_size and still has the temporary
 * buffer problem.
 *
 * So we use a special-cased scatter (MPIR_Iscatter_for_bcast) that just handles
 * bytes and knows how to deal with a "ragged edge" vector length and we
 * implement the recursive doubling algorithm here.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_scatter_rec_dbl_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_scatter_rec_dbl_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, dst;
    int relative_rank, mask;
    int scatter_size, nbytes, curr_size, incoming_count;
    int type_size, j, k, i, tmp_mask, is_contig, is_homogeneous ATTRIBUTE((unused));
    int relative_dst, dst_tree_root, my_tree_root, send_offset;
    int recv_offset, tree_root, nprocs_completed, offset;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf;
    struct MPIR_Ibcast_status *status;
    MPIR_SCHED_CHKPMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* If there is only one process, return */
    if (comm_size == 1)
        goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN) {
        is_contig = 1;
    }
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_SCHED_CHKPMEM_MALLOC(status, struct MPIR_Ibcast_status*,
                              sizeof(struct MPIR_Ibcast_status), mpi_errno, "MPI_Status");
    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case right now */

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    status->n_bytes = nbytes;
    status->curr_bytes = 0;
    if (is_contig) {
        /* contiguous and homogeneous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = (char *)buffer + true_lb;
    }
    else {
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype,
                                        tmp_buf, nbytes, MPI_BYTE, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }


    mpi_errno = MPIR_Iscatter_for_bcast(tmp_buf, root, comm_ptr, nbytes, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* this is the block size used for the scatter operation */
    scatter_size = (nbytes + comm_size - 1) / comm_size; /* ceiling division */

    /* curr_size is the amount of data that this process now has stored in
     * buffer at byte offset (relative_rank*scatter_size) */
    curr_size = MPL_MIN(scatter_size, (nbytes - (relative_rank * scatter_size)));
    if (curr_size < 0)
        curr_size = 0;
    /* curr_size bytes already inplace */
    status->curr_bytes = curr_size;

    /* initialize because the compiler can't tell that it is always initialized when used */
    incoming_count = -1;

    mask = 0x1;
    i = 0;
    while (mask < comm_size) {
        relative_dst = relative_rank ^ mask;

        dst = (relative_dst + root) % comm_size;

        /* find offset into send and recv buffers.
           zero out the least significant "i" bits of relative_rank and
           relative_dst to find root of src and dst
           subtrees. Use ranks of roots as index to send from
           and recv into  buffer */

        dst_tree_root = relative_dst >> i;
        dst_tree_root <<= i;

        my_tree_root = relative_rank >> i;
        my_tree_root <<= i;

        send_offset = my_tree_root * scatter_size;
        recv_offset = dst_tree_root * scatter_size;

        if (relative_dst < comm_size) {
            /* calculate the exact amount of data to be received */
            /* alternative */
            if ((nbytes - recv_offset) > 0)
                incoming_count = MPL_MIN((nbytes-recv_offset), (mask * scatter_size));
            else
                incoming_count = 0;

            mpi_errno = MPIR_Sched_send(((char *)tmp_buf + send_offset),
                                        curr_size, MPI_BYTE, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier */
            mpi_errno = MPIR_Sched_recv_status(((char *)tmp_buf + recv_offset),
                                        incoming_count,
                                        MPI_BYTE, dst, comm_ptr,&status->status, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
            mpi_errno = MPIR_Sched_cb(&sched_add_length, status, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            curr_size += incoming_count;
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
           so that it doesn't show up as red in the coverage tests. */

        /* --BEGIN EXPERIMENTAL-- */
        if (dst_tree_root + mask > comm_size) {
            nprocs_completed = comm_size - my_tree_root - mask;
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

            offset = scatter_size * (my_tree_root + mask);
            tmp_mask = mask >> 1;

            while (tmp_mask) {
                relative_dst = relative_rank ^ tmp_mask;
                dst = (relative_dst + root) % comm_size;

                tree_root = relative_rank >> k;
                tree_root <<= k;

                /* send only if this proc has data and destination
                   doesn't have data. */

                if ((relative_dst > relative_rank) &&
                    (relative_rank < tree_root + nprocs_completed) &&
                    (relative_dst >= tree_root + nprocs_completed))
                {
                    /* incoming_count was set in the previous
                       receive. that's the amount of data to be
                       sent now. */
                    mpi_errno = MPIR_Sched_send(((char *)tmp_buf + offset),
                                                incoming_count, MPI_BYTE, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                /* recv only if this proc. doesn't have data and sender
                   has data */
                else if ((relative_dst < relative_rank) &&
                         (relative_dst < tree_root + nprocs_completed) &&
                         (relative_rank >= tree_root + nprocs_completed))
                {
                    /* recalculate incoming_count, since not all processes will have
                     * this value */
                    if ((nbytes - offset) > 0)
                        incoming_count = MPL_MIN((nbytes-offset), (mask * scatter_size));
                    else
                        incoming_count = 0;

                    /* nprocs_completed is also equal to the no. of processes
                       whose data we don't have */
                    mpi_errno = MPIR_Sched_recv_status(((char *)tmp_buf + offset),
                                                incoming_count, MPI_BYTE, dst, comm_ptr,
                                                &status->status, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                    mpi_errno = MPIR_Sched_cb(&sched_add_length, status, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    curr_size += incoming_count;
                }
                tmp_mask >>= 1;
                k--;
            }
        }
        /* --END EXPERIMENTAL-- */

        mask <<= 1;
        i++;
    }
    if(is_homogeneous){
        mpi_errno = MPIR_Sched_cb(&sched_test_curr_length, status, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_BYTE,
                                        buffer, count, datatype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

/*
   Broadcast based on a scatter followed by an allgather.

   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   We use a ring algorithm for the allgather, which takes p-1 steps.
   This may perform better than recursive doubling for long messages and
   medium-sized non-power-of-two messages.
   Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank;
    int is_contig, is_homogeneous ATTRIBUTE((unused)), type_size, nbytes;
    int scatter_size, curr_size;
    int i, j, jnext, left, right;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf = NULL;

    struct MPIR_Ibcast_status *status;
    MPIR_SCHED_CHKPMEM_DECL(4);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1)
        goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case yet */
    MPIR_SCHED_CHKPMEM_MALLOC(status, struct MPIR_Ibcast_status*,
                              sizeof(struct MPIR_Ibcast_status), mpi_errno, "MPI_Status");
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;
    status->n_bytes = nbytes;
    status->curr_bytes = 0;
    if (is_contig) {
        /* contiguous, no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = (char *) buffer + true_lb;
    }
    else {
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    mpi_errno = MPIR_Iscatter_for_bcast(tmp_buf, root, comm_ptr, nbytes, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* this is the block size used for the scatter operation */
    scatter_size = (nbytes + comm_size - 1) / comm_size; /* ceiling division */

    /* curr_size is the amount of data that this process now has stored in
     * buffer at byte offset (rank*scatter_size) */
    curr_size = MPL_MIN(scatter_size, (nbytes - (rank * scatter_size)));
    if (curr_size < 0)
        curr_size = 0;
    /* curr_size bytes already inplace */
    status->curr_bytes = curr_size;

    /* long-message allgather or medium-size but non-power-of-two. use ring algorithm. */

    left  = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    j     = rank;
    jnext = left;
    for (i = 1; i < comm_size; i++) {
        int left_count, right_count, left_disp, right_disp, rel_j, rel_jnext;

        rel_j     = (j     - root + comm_size) % comm_size;
        rel_jnext = (jnext - root + comm_size) % comm_size;
        left_count = MPL_MIN(scatter_size, (nbytes - rel_jnext * scatter_size));
        if (left_count < 0)
            left_count = 0;
        left_disp = rel_jnext * scatter_size;
        right_count = MPL_MIN(scatter_size, (nbytes - rel_j * scatter_size));
        if (right_count < 0)
            right_count = 0;
        right_disp = rel_j * scatter_size;

        mpi_errno = MPIR_Sched_send(((char *)tmp_buf + right_disp),
                                    right_count, MPI_BYTE, right, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier here */
        mpi_errno = MPIR_Sched_recv_status(((char *)tmp_buf + left_disp),
                                    left_count, MPI_BYTE, left, comm_ptr, &status->status, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        mpi_errno = MPIR_Sched_cb(&sched_add_length, status, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        j     = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }
    mpi_errno = MPIR_Sched_cb(&sched_test_curr_length, status, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (!is_contig && rank != root) {
        mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}


/* This routine purely handles the hierarchical version of bcast, and does not
 * currently make any decision about which particular algorithm to use for any
 * subcommunicator. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_SMP
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_SMP(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_homogeneous;
    MPI_Aint type_size;
    struct MPIR_Ibcast_status *status;
    MPIR_SCHED_CHKPMEM_DECL(1);

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_BCAST)
        MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "SMP collectives are disabled!");
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));
    MPIR_SCHED_CHKPMEM_MALLOC(status, struct MPIR_Ibcast_status*,
                              sizeof(struct MPIR_Ibcast_status), mpi_errno, "MPI_Status");

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case yet */
    if (comm_ptr->node_comm) {
        MPIR_Assert(comm_ptr->node_comm->coll_fns);
        MPIR_Assert(comm_ptr->node_comm->coll_fns->Ibcast_sched);
    }
    if (comm_ptr->node_roots_comm) {
        MPIR_Assert(comm_ptr->node_roots_comm->coll_fns);
        MPIR_Assert(comm_ptr->node_roots_comm->coll_fns->Ibcast_sched);
    }

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPIR_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

     status->n_bytes = type_size * count;
    /* TODO insert packing here */

    /* send to intranode-rank 0 on the root's node */
    if (comm_ptr->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
    {                                                /* and is on our node (!-1) */
        if (root == comm_ptr->rank) {
            mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        }
        else if (0 == comm_ptr->node_comm->rank) {
            mpi_errno = MPIR_Sched_recv_status(buffer, count, datatype, MPIR_Get_intranode_rank(comm_ptr, root),
                                        comm_ptr->node_comm, &status->status, s);
        }
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        MPIR_SCHED_BARRIER(s);
        mpi_errno = MPIR_Sched_cb(&sched_test_length, status, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* perform the internode broadcast */
    if (comm_ptr->node_roots_comm != NULL)
    {
        mpi_errno = comm_ptr->node_roots_comm->coll_fns->Ibcast_sched(buffer, count, datatype,
                                                                MPIR_Get_internode_rank(comm_ptr, root),
                                                                comm_ptr->node_roots_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* don't allow the local ops for the intranode phase to start until this has completed */
        MPIR_SCHED_BARRIER(s);
    }
    /* perform the intranode broadcast on all except for the root's node */
    if (comm_ptr->node_comm != NULL)
    {
        mpi_errno = comm_ptr->node_comm->coll_fns->Ibcast_sched(buffer, count, datatype, 0, comm_ptr->node_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}


/* Provides a generic "flat" broadcast that doesn't know anything about hierarchy.  It will choose
 * between several different algorithms based on the given parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_intra(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, is_homogeneous ATTRIBUTE((unused));
    MPI_Aint type_size, nbytes;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    MPIR_Assert(is_homogeneous); /* we don't handle the hetero case right now */

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm_size < MPIR_CVAR_BCAST_MIN_PROCS))
    {
        mpi_errno = MPIR_Ibcast_binomial(buffer, count, datatype, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */
    {
        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPIU_is_pof2(comm_size, NULL))) {
            mpi_errno = MPIR_Ibcast_scatter_rec_dbl_allgather(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            mpi_errno = MPIR_Ibcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Provides a generic "flat" broadcast for intercommunicators that doesn't know
 * anything about hierarchy.  It will choose between several different
 * algorithms based on the given parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_inter(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    /* Intercommunicator broadcast.
     * Root sends to rank 0 in remote group. Remote group does local
     * intracommunicator broadcast. */
    if (root == MPI_PROC_NULL)
    {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    }
    else if (root == MPI_ROOT)
    {
        /* root sends to rank 0 on remote group and returns */
        mpi_errno = MPIR_Sched_send(buffer, count, datatype, 0, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else
    {
        /* remote group. rank 0 on remote group receives from root */
        if (comm_ptr->rank == 0) {
            mpi_errno = MPIR_Sched_recv(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

        if (comm_ptr->local_comm == NULL) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        /* now do the usual broadcast on this intracommunicator
           with rank 0 as root. */
        MPIR_Assert(comm_ptr->local_comm->coll_fns && comm_ptr->local_comm->coll_fns->Ibcast_sched);
        mpi_errno = comm_ptr->local_comm->coll_fns->Ibcast_sched(buffer, count, datatype, root, comm_ptr->local_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPI_Request *request)
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

    MPIR_Assert(comm_ptr->coll_fns->Ibcast_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Ibcast_sched(buffer, count, datatype, root, comm_ptr, s);
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
#define FUNCNAME MPI_Ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ibcast - Broadcasts a message from the process with rank "root" to
             all other processes of the communicator in a nonblocking way

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (non-negative integer)
. datatype - datatype of buffer (handle)
. root - rank of broadcast root (integer)
- comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBCAST);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IBCAST);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_COUNT(count, mpi_errno);
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

            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Ibcast(buffer, count, datatype, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IBCAST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ibcast", "**mpi_ibcast %p %d %D %C %p", buffer, count, datatype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
