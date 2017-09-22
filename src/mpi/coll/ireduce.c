/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ireduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ireduce = PMPI_Ireduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ireduce  MPI_Ireduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ireduce as PMPI_Ireduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
                __attribute__((weak,alias("PMPI_Ireduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ireduce
#define MPI_Ireduce PMPI_Ireduce

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_binomial(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank, is_commutative;
    int mask, relrank, source, lroot;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    MPIR_SCHED_CHKPMEM_DECL(2);

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (count == 0) return MPI_SUCCESS;

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

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                        mpi_errno, "temporary buffer");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
        MPIR_SCHED_CHKPMEM_MALLOC(recvbuf, void *,
                            count*(MPL_MAX(extent,true_extent)),
                            mpi_errno, "receive buffer");
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        /* could do this up front as an MPIR_Localcopy instead, but we'll defer
         * it to the progress engine */
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype, recvbuf, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* This code is from MPICH-1. */

    /* Here's the algorithm.  Relative to the root, look at the bit pattern in
       my rank.  Starting from the right (lsb), if the bit is 1, send to
       the node with that bit zero and exit; if the bit is 0, receive from the
       node with that bit set and combine (as long as that node is within the
       group)

       Note that by receiving with source selection, we guarentee that we get
       the same bits with the same input.  If we allowed the parent to receive
       the children in any order, then timing differences could cause different
       results (roundoff error, over/underflows in some cases, etc).

       Because of the way these are ordered, if root is 0, then this is correct
       for both commutative and non-commutitive operations.  If root is not
       0, then for non-commutitive, we use a root of zero and then send
       the result to the root.  To see this, note that the ordering is
       mask = 1: (ab)(cd)(ef)(gh)            (odds send to evens)
       mask = 2: ((ab)(cd))((ef)(gh))        (3,6 send to 0,4)
       mask = 4: (((ab)(cd))((ef)(gh)))      (4 sends to 0)

       Comments on buffering.
       If the datatype is not contiguous, we still need to pass contiguous
       data to the user routine.
       In this case, we should make a copy of the data in some format,
       and send/operate on that.

       In general, we can't use MPI_PACK, because the alignment of that
       is rather vague, and the data may not be re-usable.  What we actually
       need is a "squeeze" operation that removes the skips.
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
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_barrier(s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                /* The sender is above us, so the received buffer must be
                   the second argument (in the noncommutative case). */
                if (is_commutative) {
                    mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    mpi_errno = MPIR_Sched_reduce(recvbuf, tmp_buf, count, datatype, op, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_barrier(s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_copy(tmp_buf, count, datatype, recvbuf, count, datatype, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                mpi_errno = MPIR_Sched_barrier(s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
        else {
            /* I've received all that I'm going to.  Send my result to
               my parent */
            source = ((relrank & (~ mask)) + lroot) % comm_size;
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, source, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            break;
        }
        mask <<= 1;
    }

    if (!is_commutative && (root != 0))
    {
        if (rank == 0)
        {
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else if (rank == root)
        {
            mpi_errno = MPIR_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
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

/* An implementation of Rabenseifner's reduce algorithm (see
   http://www.hlrs.de/mpi/myreduce.html).

   This algorithm implements the reduce in two steps: first a
   reduce-scatter, followed by a gather to the root. A
   recursive-halving algorithm (beginning with processes that are
   distance 1 apart) is used for the reduce-scatter, and a binomial tree
   algorithm is used for the gather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few odd-numbered processes send their data to their left neighbors
   (rank-1), and the reduce-scatter happens among the remaining
   power-of-two processes. If the root is one of the excluded
   processes, then after the reduce-scatter, rank 0 sends its result to
   the root and exits; the root now acts as rank 0 in the binomial tree
   algorithm for gather.

   For the power-of-two case, the cost for the reduce-scatter is
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   gather to root is lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, assuming the root is not one of the
   odd-numbered processes that get excluded in the reduce-scatter,
   Cost = (2.floor(lgp)+1).alpha + (2.((p-1)/p) + 1).n.beta +
           n.(1+(p-1)/p).gamma
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_redscat_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_redscat_gather(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j, comm_size, rank, pof2, is_commutative ATTRIBUTE((unused));
    int rem, dst, newrank, newdst, mask, send_idx, recv_idx, last_idx;
    int send_cnt, recv_cnt, newroot, newdst_tree_root, newroot_tree_root;
    void *tmp_buf = NULL;
    int *cnts, *disps;
    MPI_Aint true_lb, true_extent, extent;
    MPIR_SCHED_CHKPMEM_DECL(2);
    MPIR_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* NOTE: this algorithm is currently only correct for commutative operations */
    is_commutative = MPIR_Op_is_commutative(op);
    MPIR_Assert(is_commutative);

    /* Create a temporary buffer */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    /* I think this is the worse case, so we can avoid an assert()
     * inside the for loop */
    /* should be buf+{this}? */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                              mpi_errno, "temporary buffer");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    rem = comm_size - pof2;

    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
        MPIR_SCHED_CHKPMEM_MALLOC(recvbuf, void *, count*(MPL_MAX(extent,true_extent)),
                                  mpi_errno, "receive buffer");
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype,
                                    recvbuf, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send their data to
       (rank-1). These odd-numbered processes no longer
       participate in the algorithm until the very end. The
       remaining processes form a nice power-of-two.

       Note that in MPI_Allreduce we have the even-numbered processes
       send data to odd-numbered processes. That is better for
       non-commutative operations because it doesn't require a
       buffer copy. However, for MPI_Reduce, the most common case
       is commutative operations with root=0. Therefore we want
       even-numbered processes to participate the computation for
       the root=0 case, in order to avoid an extra send-to-root
       communication after the reduce-scatter. In MPI_Allreduce it
       doesn't matter because all processes must get the result. */

    if (rank < 2*rem) {
        if (rank % 2 != 0) { /* odd */
            mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, rank-1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* even */
            mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, rank+1, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* do the reduction on received data. */
            /* This algorithm is used only for predefined ops
               and predefined ops are always commutative. */
            mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else { /* rank >= 2*rem */
        newrank = rank - rem;
    }

    /* for the reduce-scatter, calculate the count that
       each process receives and the displacement within
       the buffer */

    /* We allocate these arrays on all processes, even if newrank=-1,
       because if root is one of the excluded processes, we will
       need them on the root later on below. */
    MPIR_CHKLMEM_MALLOC(cnts, int *, pof2*sizeof(int), mpi_errno, "counts");
    MPIR_CHKLMEM_MALLOC(disps, int *, pof2*sizeof(int), mpi_errno, "displacements");

    last_idx = send_idx = 0; /* suppress spurious compiler warnings */

    if (newrank != -1) {
        for (i=0; i<(pof2-1); i++)
            cnts[i] = count/pof2;
        cnts[pof2-1] = count - (count/pof2)*(pof2-1);

        disps[0] = 0;
        for (i=1; i<pof2; i++)
            disps[i] = disps[i-1] + cnts[i-1];

        mask = 0x1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst*2 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + pof2/(mask*2);
                for (i=send_idx; i<last_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<send_idx; i++)
                    recv_cnt += cnts[i];
            }
            else {
                recv_idx = send_idx + pof2/(mask*2);
                for (i=send_idx; i<recv_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<last_idx; i++)
                    recv_cnt += cnts[i];
            }

            /* Send data from recvbuf. Recv into tmp_buf */
            mpi_errno = MPIR_Sched_send(((char *)recvbuf + disps[send_idx]*extent),
                                        send_cnt, datatype,
                                        dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPIR_Sched_recv(((char *)tmp_buf + disps[recv_idx]*extent),
                                        recv_cnt, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* tmp_buf contains data received in this step.
               recvbuf contains data accumulated so far */

            /* This algorithm is used only for predefined ops
               and predefined ops are always commutative. */
            mpi_errno = MPIR_Sched_reduce(((char *)tmp_buf + disps[recv_idx]*extent),
                                          ((char *)recvbuf + disps[recv_idx]*extent),
                                          recv_cnt, datatype, op, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            /* update send_idx for next iteration */
            send_idx = recv_idx;
            mask <<= 1;

            /* update last_idx, but not in last iteration
               because the value is needed in the gather
               step below. */
            if (mask < pof2)
                last_idx = recv_idx + pof2/mask;
        }
    }

    /* now do the gather to root */

    /* Is root one of the processes that was excluded from the
       computation above? If so, send data from newrank=0 to
       the root and have root take on the role of newrank = 0 */

    if (root < 2*rem) {
        if (root % 2 != 0) {
            if (rank == root) {    /* recv */
                /* initialize the arrays that weren't initialized */
                for (i=0; i<(pof2-1); i++)
                    cnts[i] = count/pof2;
                cnts[pof2-1] = count - (count/pof2)*(pof2-1);

                disps[0] = 0;
                for (i=1; i<pof2; i++)
                    disps[i] = disps[i-1] + cnts[i-1];

                mpi_errno = MPIR_Sched_recv(recvbuf, cnts[0], datatype, 0, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                newrank = 0;
                send_idx = 0;
                last_idx = 2;
            }
            else if (newrank == 0) {  /* send */
                mpi_errno = MPIR_Sched_send(recvbuf, cnts[0], datatype, root, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                newrank = -1;
            }
            newroot = 0;
        }
        else newroot = root / 2;
    }
    else {
        newroot = root - rem;
    }

    if (newrank != -1) {
        j = 0;
        mask = 0x1;
        while (mask < pof2) {
            mask <<= 1;
            j++;
        }
        mask >>= 1;
        j--;
        while (mask > 0) {
            newdst = newrank ^ mask;

            /* find real rank of dest */
            dst = (newdst < rem) ? newdst*2 : newdst + rem;
            /* if root is playing the role of newdst=0, adjust for
               it */
            if ((newdst == 0) && (root < 2*rem) && (root % 2 != 0))
                dst = root;

            /* if the root of newdst's half of the tree is the
               same as the root of newroot's half of the tree, send to
               newdst and exit, else receive from newdst. */

            newdst_tree_root = newdst >> j;
            newdst_tree_root <<= j;

            newroot_tree_root = newroot >> j;
            newroot_tree_root <<= j;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                /* update last_idx except on first iteration */
                if (mask != pof2/2)
                    last_idx = last_idx + pof2/(mask*2);

                recv_idx = send_idx + pof2/(mask*2);
                for (i=send_idx; i<recv_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<last_idx; i++)
                    recv_cnt += cnts[i];
            }
            else {
                recv_idx = send_idx - pof2/(mask*2);
                for (i=send_idx; i<last_idx; i++)
                    send_cnt += cnts[i];
                for (i=recv_idx; i<send_idx; i++)
                    recv_cnt += cnts[i];
            }

            if (newdst_tree_root == newroot_tree_root) {
                /* send and exit */
                /* Send data from recvbuf. Recv into tmp_buf */
                mpi_errno = MPIR_Sched_send(((char *)recvbuf + disps[send_idx]*extent),
                                            send_cnt, datatype, dst, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
                break;
            }
            else {
                /* recv and continue */
                mpi_errno = MPIR_Sched_recv(((char *)recvbuf + disps[recv_idx]*extent),
                                            recv_cnt, datatype, dst, comm_ptr, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }

            if (newrank > newdst) send_idx = recv_idx;

            mask >>= 1;
            j--;
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_intra(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2, type_size, comm_size;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    if ((count*type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) &&
        (count >= pof2))
    {
        /* do a reduce-scatter followed by gather to root. */
        mpi_errno = MPIR_Ireduce_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* use a binomial tree algorithm */
        mpi_errno = MPIR_Ireduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_SMP
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_SMP(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    MPI_Aint  true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    MPIR_Comm *nc;
    MPIR_Comm *nrc;
    MPIR_SCHED_CHKPMEM_DECL(1);

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_REDUCE)
        MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "SMP collectives are disabled!");
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));
    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    nc = comm_ptr->node_comm;
    nrc = comm_ptr->node_roots_comm;

    /* is the op commutative? We do SMP optimizations only if it is. */
    is_commutative = MPIR_Op_is_commutative(op);
    if (!is_commutative) {
        mpi_errno = MPIR_Ireduce_intra(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* Create a temporary buffer on local roots of all nodes */
    if (nrc != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                                  mpi_errno, "temporary buffer");
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (nc != NULL && MPIR_Get_intranode_rank(comm_ptr, root) == -1) {
        MPIR_Assert(nc->coll_fns && nc->coll_fns->Ireduce_sched);
        mpi_errno = nc->coll_fns->Ireduce_sched(sendbuf, tmp_buf, count, datatype, op, 0, nc, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* do the internode reduce to the root's node */
    if (nrc != NULL) {
        MPIR_Assert(nrc->coll_fns && nrc->coll_fns->Ireduce_sched);
        if (nrc->rank != MPIR_Get_internode_rank(comm_ptr, root)) {
            /* I am not on root's node.  Use tmp_buf if we
               participated in the first reduce, otherwise use sendbuf */
            const void *buf = (nc == NULL ? sendbuf : tmp_buf);
            mpi_errno = nrc->coll_fns->Ireduce_sched(buf, NULL, count, datatype,
                                               op, MPIR_Get_internode_rank(comm_ptr, root),
                                               nrc, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        else { /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm_ptr->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                   Use tmp_buf as recvbuf. */

                mpi_errno = nrc->coll_fns->Ireduce_sched(sendbuf, tmp_buf, count, datatype,
                                                   op, MPIR_Get_internode_rank(comm_ptr, root),
                                                   nrc, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            }
            else {
                /* I am the root. in_place is automatically handled. */

                mpi_errno = nrc->coll_fns->Ireduce_sched(sendbuf, recvbuf, count, datatype,
                                                   op, MPIR_Get_internode_rank(comm_ptr, root),
                                                   nrc, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }
    }

    /* do the intranode reduce on the root's node */
    if (nc != NULL && MPIR_Get_intranode_rank(comm_ptr, root) != -1) {
        mpi_errno = nc->coll_fns->Ireduce_sched(sendbuf, recvbuf, count, datatype,
                                          op, MPIR_Get_intranode_rank(comm_ptr, root),
                                          nc, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
#define FUNCNAME MPIR_Ireduce_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_inter(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(1);

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

/*  Intercommunicator reduce.
    Remote group does a local intracommunicator
    reduce to rank 0. Rank 0 then sends data to root.
*/

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIR_Sched_recv(recvbuf, count, datatype, 0, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* remote group. Rank 0 allocates temporary buffer, does
           local intracommunicator reduce, and then sends the data
           to root. */
        rank = comm_ptr->rank;

        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            MPIR_Datatype_get_extent_macro(datatype, extent);
            /* I think this is the worse case, so we can avoid an assert()
             * inside the for loop */
            /* Should MPIR_SCHED_CHKPMEM_MALLOC do this? */
            MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
            MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);
        }

        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIR_Ireduce_intra(sendbuf, tmp_buf, count, datatype, op, 0, comm_ptr->local_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_barrier(s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (rank == 0) {
            mpi_errno = MPIR_Sched_send(tmp_buf, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            mpi_errno = MPIR_Sched_barrier(s);
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

#undef FUNCNAME
#define FUNCNAME MPIR_Ireduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ireduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm *comm_ptr, MPI_Request *request)
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
    MPIR_Assert(comm_ptr->coll_fns->Ireduce_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
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
#define FUNCNAME MPI_Ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ireduce - Reduces values on all processes to a single value
              in a nonblocking way

Input Parameters:
+ sendbuf - address of the send buffer (choice)
. count - number of elements in send buffer (non-negative integer)
. datatype - data type of elements of send buffer (handle)
. op - reduce operation (handle)
. root - rank of root process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IREDUCE);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IREDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
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
            int rank;

            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op *op_ptr = NULL;
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            }
            else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype);
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                if (sendbuf != MPI_IN_PLACE)
                    MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

                rank = comm_ptr->rank;
                if (rank == root) {
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);
                    if (count != 0 && sendbuf != MPI_IN_PLACE) {
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
                    }
                }
                else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
            }

            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IREDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ireduce", "**mpi_ireduce %p %p %d %D %O %d %C %p", sendbuf, recvbuf, count, datatype, op, root, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
