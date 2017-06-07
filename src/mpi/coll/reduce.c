/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_REDUCE_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the short message algorithm will be used if the send buffer size is <=
        this value (in bytes)

    - name        : MPIR_CVAR_ENABLE_SMP_REDUCE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware reduce.

    - name        : MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware reduce is used.  A
        value of '0' uses SMP-aware reduce for all message sizes.


=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Reduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce = PMPI_Reduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce  MPI_Reduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce as PMPI_Reduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm)
               __attribute__((weak,alias("PMPI_Reduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce
#define MPI_Reduce PMPI_Reduce

/* This function implements a binomial tree reduce.

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Reduce_binomial ( 
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    int comm_size, rank, is_commutative, type_size ATTRIBUTE((unused));
    int mask, relrank, source, lroot;
    MPI_Aint true_lb, true_extent, extent; 
    void *tmp_buf;
    MPIR_CHKLMEM_DECL(2);

    if (count == 0) return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Create a temporary buffer */

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(datatype, extent);

    is_commutative = MPIR_Op_is_commutative(op);

    /* I think this is the worse case, so we can avoid an assert() 
     * inside the for loop */
    /* should be buf+{this}? */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                        mpi_errno, "temporary buffer");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
        MPIR_CHKLMEM_MALLOC(recvbuf, void *,
                            count*(MPL_MAX(extent,true_extent)), 
                            mpi_errno, "receive buffer");
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
        if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
    }

    MPID_Datatype_get_size_macro(datatype, type_size);

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
    mask    = 0x1;
    if (is_commutative) 
        lroot   = root;
    else
        lroot   = 0;
    relrank = (rank - lroot + comm_size) % comm_size;
    
    while (/*(mask & relrank) == 0 && */mask < comm_size) {
        /* Receive */
        if ((mask & relrank) == 0) {
            source = (relrank | mask);
            if (source < comm_size) {
                source = (source + lroot) % comm_size;
                mpi_errno = MPIC_Recv(tmp_buf, count, datatype, source,
                                         MPIR_REDUCE_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* The sender is above us, so the received buffer must be
                   the second argument (in the noncommutative case). */
                if (is_commutative) {
                    mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    mpi_errno = MPIR_Reduce_local_impl(recvbuf, tmp_buf, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               recvbuf, count, datatype);
                    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
                }
            }
        }
        else {
            /* I've received all that I'm going to.  Send my result to 
               my parent */
            source = ((relrank & (~ mask)) + lroot) % comm_size;
            mpi_errno  = MPIC_Send(recvbuf, count, datatype,
                                      source, MPIR_REDUCE_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            break;
        }
        mask <<= 1;
    }

    if (!is_commutative && (root != 0))
    {
        if (rank == 0)
        {
            mpi_errno  = MPIC_Send(recvbuf, count, datatype, root,
                                      MPIR_REDUCE_TAG, comm_ptr, errflag);
        }
        else if (rank == root)
        {
            mpi_errno = MPIC_Recv(recvbuf, count, datatype, 0,
                                    MPIR_REDUCE_TAG, comm_ptr, &status, errflag);
        }
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* An implementation of Rabenseifner's reduce algorithm [1, 2].

   [1] Rajeev Thakur, Rolf Rabenseifner and William Gropp.
       Optimization of Collective Communication Operations in MPICH //
       The Int. Journal of High Performance Computing Applications. Vol 19,
       Issue 1, pp. 49--66.
   [2] http://www.hlrs.de/mpi/myreduce.html

  This algorithm is a combination of a reduce-scatter implemented with
  recursive vector halving and recursive distance doubling, followed either
  by a binomial tree gather.

  Step 1. If the number of processes is not a power of two, reduce it to
  the nearest lower power of two (p' = 2^{\lfloor\log_2 p\rfloor})
  by removing r = p - p' extra processes as follows. In the first 2r processes
  (ranks 0 to 2r - 1), all the even ranks send the second half of the input
  vector to their right neighbor (rank + 1), and all the odd ranks send
  the first half of the input vector to their left neighbor (rank - 1).
  The even ranks compute the reduction on the first half of the vector and
  the odd ranks compute the reduction on the second half. The odd ranks then
  send the result to their left neighbors (the even ranks). As a result,
  the even ranks among the first 2r processes now contain the reduction with
  the input vector on their right neighbors (the odd ranks). These odd ranks
  do not participate in the rest of the algorithm, which leaves behind
  a power-of-two number of processes. The first r even-ranked processes and
  the last p - 2r processes are now renumbered from 0 to p' - 1.

  Step 2. The remaining processes now perform a reduce-scatter by using
  recursive vector halving and recursive distance doubling. The even-ranked
  processes send the second half of their buffer to rank + 1 and the odd-ranked
  processes send the first half of their buffer to rank - 1. All processes
  then compute the reduction between the local buffer and the received buffer.
  In the next \log_2(p') - 1 steps, the buffers are recursively halved, and the
  distance is doubled. At the end, each of the p' processes has 1 / p' of the
  total reduction result.

  Step 3. A binomial tree gather is performed by using recursive vector
  doubling and distance halving. In the non-power-of-two case, if the root
  happens to be one of those odd-ranked processes that would normally
  be removed in the first step, then the role of this process and process 0
  are interchanged.

  Limitations: commutative operations only, count >= 2^{\lfloor\log_2 p\rfloor}
  Recommendations: root = 0, otherwise it is required additional steps
                   in the root process.

  Memory consumption (per process):
  1) rank != root: 2 * count * dtypesize + 4 * \log_2(p) * sizeof(int)
  2) rank == root: count * dtypesize + 4 * \log_2(p) * sizeof(int)

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
#define FUNCNAME MPIR_Reduce_redscat_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Reduce_redscat_gather (
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size, rank, type_size ATTRIBUTE((unused)), pof2, rem, newrank;
    int mask, dst, newdst, nsteps, step, wsize;
    int newroot, newdst_tree_root, newroot_tree_root;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    int *rindex, *rcount, *sindex, *scount, count_lhalf, count_rhalf;

    MPIR_CHKLMEM_DECL(6);

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
    MPID_Datatype_get_extent_macro(datatype, extent);

    /* I think this is the worse case, so we can avoid an assert()
     * inside the for loop */
    /* should be buf+{this}? */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                        mpi_errno, "temporary buffer");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* If I'm not the root, then my recvbuf may not be valid, therefore
       I have to allocate a temporary one */
    if (rank != root) {
        MPIR_CHKLMEM_MALLOC(recvbuf, void *,
                            count*(MPL_MAX(extent,true_extent)),
                            mpi_errno, "receive buffer");
        recvbuf = (void *)((char*)recvbuf - true_lb);
    }

    if ((rank != root) || (sendbuf != MPI_IN_PLACE)) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
        if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
    }

    MPID_Datatype_get_size_macro(datatype, type_size);

    /* Step 1. Reduce the number of processes to the nearest lower power of two
     * (p' = 2^{\lfloor\log_2 p\rfloor}) by removing r = p - p' processes.
     * 1. In the first 2r processes (ranks 0 to 2r - 1), all the even ranks send
     *    the second half of the input vector to their right neighbor (rank + 1)
     *    and all the odd ranks send the first half of the input vector to their
     *    left neighbor (rank - 1).
     * 2. All 2r processes compute the reduction on their half.
     * 3. The odd ranks then send the result to their left neighbors
     *    (the even ranks).
     *
     * The even ranks (0 to 2r - 1) now contain the reduction with the input
     * vector on their right neighbors (the odd ranks). The first r even
     * processes and the p - 2r last processes are renumbered from
     * 0 to 2^{\floor(log_2 p)} - 1. These odd ranks do not participate in the
     * rest of the algorithm.
     */

    /* Find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    nsteps = -1;
    while (pof2 <= comm_size) {  /* O(\log_2(p)), TODO: use flp2 and ilog2 */
        pof2 <<= 1;
        nsteps++;
    }
    pof2 >>= 1;

    rem = comm_size - pof2;
    if (rank < 2 * rem) {
        count_lhalf = count / 2;
        count_rhalf = count - count_lhalf;

        if (rank % 2 != 0) {
            /*
             * Odd process -- exchange with rank - 1
             * Send the left half of the input vector to the left neighbor,
             * Recv the right half of the input vector from the left neighbor
             */
            mpi_errno = MPIC_Sendrecv(recvbuf, count_lhalf, datatype,
                                      rank - 1, MPIR_REDUCE_TAG,
                                      (char *)tmp_buf + count_lhalf * extent,
                                      count_rhalf, datatype, rank - 1,
                                      MPIR_REDUCE_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* Reduce on the right half of the buffers (result in recvbuf) */
            mpi_errno = MPIR_Reduce_local_impl((char *)tmp_buf +
                                               count_lhalf * extent,
                                               (char *)recvbuf +
                                               count_lhalf * extent,
                                               count_rhalf, datatype, op);

            /* Send the right half to the left neighbor */
            mpi_errno = MPIC_Send((char *)recvbuf + count_lhalf * extent,
                                  count_rhalf, datatype, rank - 1,
                                  MPIR_REDUCE_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            /* Temporarily set the rank to -1 so that this process does not
               pariticipate in recursive doubling */
            newrank = -1;

        } else {
            /*
             * Even process -- exchange with rank + 1
             * Send the right half of the input vector to the right neighbor,
             * Recv the left half of the input vector from the right neighbor
             */
            mpi_errno = MPIC_Sendrecv((char *)recvbuf + count_lhalf * extent,
                                      count_rhalf, datatype, rank + 1,
                                      MPIR_REDUCE_TAG, tmp_buf, count_lhalf,
                                      datatype, rank + 1, MPIR_REDUCE_TAG,
                                      comm_ptr, MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* Reduce on the left half of the buffers (result in recvbuf) */
            mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count_lhalf,
                                               datatype, op);

            /* Recv the right half from the right neighbor */
            mpi_errno = MPIC_Recv((char *)recvbuf + count_lhalf * extent,
                                  count_rhalf, datatype, rank + 1,
                                  MPIR_REDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE,
                                  errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            newrank = rank / 2;
        }
    } else { /* rank >= 2 * rem */
        newrank = rank - rem;
    }

    /*
     * Step 2. Reduce-scatter implemented with recursive vector halving and
     * recursive distance doubling. We have p' = 2^{\lfloor\log_2 p\rfloor}
     * power-of-two number of processes with new ranks and result in recvbuf.
     *
     * The even-ranked processes send the right half of their buffer to rank + 1
     * and the odd-ranked processes send the left half of their buffer to
     * rank - 1. All processes then compute the reduction between the local
     * buffer and the received buffer. In the next \log_2(p') - 1 steps, the
     * buffers are recursively halved, and the distance is doubled. At the end,
     * each of the p' processes has 1 / p' of the total reduction result.
     */

    /* We allocate these arrays on all processes, even if newrank = -1,
       because if root is one of the excluded processes, we will
       need them on the root later on below. */
    MPIR_CHKLMEM_MALLOC(rindex, int *, nsteps * sizeof(*rindex), mpi_errno,
                        "rindex buffer");
    MPIR_CHKLMEM_MALLOC(rcount, int *, nsteps * sizeof(*rcount), mpi_errno,
                        "rcount buffer");
    MPIR_CHKLMEM_MALLOC(sindex, int *, nsteps * sizeof(*sindex), mpi_errno,
                        "sindex buffer");
    MPIR_CHKLMEM_MALLOC(scount, int *, nsteps * sizeof(*scount), mpi_errno,
                        "scount buffer");

    if (newrank != -1) {
        step = 0;
        wsize = count;
        sindex[0] = rindex[0] = 0;

        for (mask = 1; mask < pof2; mask <<= 1) {
            /*
             * On each iteration: rindex[step] = sindex[step] -- begining of the
             * current window. Length of the current window is storded in wsize.
             */
            newdst = newrank ^ mask;
            /* Find real rank of the dest */
            dst = (newdst < rem) ? newdst * 2 : newdst + rem;

            if (rank < dst) {
                /*
                 * Recv into the left half of the current window, send the right
                 * half of the window to the peer (perform reduce on the left
                 * half of the current window)
                 */
                rcount[step] = wsize / 2;
                scount[step] = wsize - rcount[step];
                sindex[step] = rindex[step] + rcount[step];
            } else {
                /*
                 * Recv into the right half of the current window, send the left
                 * half of the window to the peer (perform reduce on the right
                 * half of the current window)
                 */
                scount[step] = wsize / 2;
                rcount[step] = wsize - scount[step];
                rindex[step] = sindex[step] + scount[step];
            }

            /* Send part of data from the recvbuf, recv into the tmp_buf */
            mpi_errno = MPIC_Sendrecv((char *)recvbuf + sindex[step] * extent,
                                      scount[step], datatype, dst,
                                      MPIR_REDUCE_TAG,
                                      (char *)tmp_buf + rindex[step] * extent,
                                      rcount[step], datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* Local reduce: recvbuf[] = tmp_buf[] <op> recvbuf[] */
            mpi_errno = MPIR_Reduce_local_impl((char *)tmp_buf +
                                               rindex[step] * extent,
                                               (char *)recvbuf +
                                               rindex[step] * extent,
                                               rcount[step], datatype, op);

            /* Move the current window to the received message */
            if (step + 1 < nsteps) {
                rindex[step + 1] = rindex[step];
                sindex[step + 1] = rindex[step];
                wsize = rcount[step];
                step++;
            }
        }
    }
    /*
     * Assertion: each process has 1 / p' of the total reduction result:
     * rcount[nsteps - 1] elements in the recvbuf[rindex[nsteps - 1]...].
     */

    /*
     * Now do the gather to root.
     * Setup the root process for gather operation.
     * Case 1: root < 2r and root is odd -- root process was excluded on step 1
     *         Recv data from process 0, newroot = 0, newrank = 0
     * Case 2: root < 2r and root is even: newroot = root / 2
     * Case 3: root >= 2r: newroot = root - r
     */
    newroot = 0;
    if (root < 2 * rem) {
        if (root % 2 != 0) {
            newroot = 0;
            if (rank == root) {
                /*
                 * Case 1: root < 2r and root is odd -- root process was
                 * excluded on step 1 (newrank == -1).
                 * Recv a data from the process 0.
                 */
                rindex[0] = 0;
                step = 0, wsize = count;
                for (mask = 1; mask < pof2; mask *= 2) {
                    rcount[step] = wsize / 2;
                    scount[step] = wsize - rcount[step];
                    rindex[step] = 0;
                    sindex[step] = rcount[step];
                    step++;
                    wsize /= 2;
                }
                mpi_errno = MPIC_Recv(recvbuf, rcount[nsteps - 1], datatype, 0,
                                      MPIR_REDUCE_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                newrank = 0;
            } else if (newrank == 0) {
                /* Send a data to the root */
                mpi_errno = MPIC_Send(recvbuf, rcount[nsteps - 1], datatype,
                                      root, MPIR_REDUCE_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                newrank = -1;
            }
        } else {
            /* Case 2: root < 2r and a root is even: newroot = root / 2 */
            newroot = root / 2;
        }
    } else {
        /* Case 3: root >= 2r: newroot = root - r */
        newroot = root - rem;
    }

    /*
     * Step 3. Gather result at the newroot by the binomial tree algorithm.
     * Each process has 1 / p' of the total reduction result:
     * rcount[nsteps - 1] elements in the recvbuf[rindex[nsteps - 1]...].
     * All exchanges are executed in reverse order relative
     * to recursive doubling (previous step).
     */
    if (newrank != -1) {
        mask = pof2 >> 1;
        step = nsteps - 1; /* step = \ilog_2(p') - 1 */

        while (mask > 0) {
            newdst = newrank ^ mask;
            /* Find real rank of the dest */
            dst = (newdst < rem) ? newdst * 2 : newdst + rem;
            /* If root is playing the role of newdst = 0, adjust for it */
            if ((newdst == 0) && (root < 2 * rem) && (root % 2 != 0))
                dst = root;

            /* If the root of newdst's half of the tree is the
               same as the root of newroot's half of the tree,
               send to newdst and exit, else receive from newdst. */
            newdst_tree_root = newdst >> step;
            newdst_tree_root <<= step;
            newroot_tree_root = newroot >> step;
            newroot_tree_root <<= step;

            if (newdst_tree_root == newroot_tree_root) {
                /* Send data from recvbuf and exit */
                mpi_errno = MPIC_Send((char *)recvbuf + rindex[step] * extent,
                                      rcount[step], datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                break;
            } else {
                /* Recv and continue */
                mpi_errno = MPIC_Recv((char *)recvbuf + sindex[step] * extent,
                                      scount[step], datatype, dst,
                                      MPIR_REDUCE_TAG, comm_ptr,
                                      MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            step--;
            mask >>= 1;
        }
    }

    /* FIXME does this need to be checked after each uop invocation for
       predefined operators? */
    /* --BEGIN ERROR HANDLING-- */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno) {
            mpi_errno = per_thread->op_errno;
            goto fn_fail;
        }
    }
    /* --END ERROR HANDLING-- */

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This is the default implementation of reduce. The algorithm is:
   
   Algorithm: MPI_Reduce

   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see 
   http://www.hlrs.de/organization/par/services/models/mpi/myreduce.html ).
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


   For short messages, user-defined ops, and count < pof2, we use a
   binomial tree algorithm for both short and long messages. 

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma


   We use the binomial tree algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky.

   FIXME: Per the MPI-2.1 standard this case is not possible.  We
   should be able to use the reduce-scatter/gather approach as long as
   count >= pof2.  [goodell@ 2009-01-21]

   Possible improvements: 

   End Algorithm: MPI_Reduce
*/


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_intra ( 
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size, is_commutative, type_size, pof2;
    int nbytes = 0;
    MPIR_Op *op_ptr;
    MPIR_CHKLMEM_DECL(1);

    if (count == 0) return MPI_SUCCESS;

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_REDUCE) {
    /* is the op commutative? We do SMP optimizations only if it is. */
    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
        is_commutative = 1;
    else {
        MPIR_Op_get_ptr(op, op_ptr);
        is_commutative = (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE) ? 0 : 1;
    }

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE ? type_size*count : 0;
    if (MPIR_Comm_is_node_aware(comm_ptr) && is_commutative &&
        nbytes <= MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE) {

        void *tmp_buf = NULL;
        MPI_Aint  true_lb, true_extent, extent;

        /* Create a temporary buffer on local roots of all nodes */
        if (comm_ptr->node_roots_comm != NULL) {

            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
            MPID_Datatype_get_extent_macro(datatype, extent);

            MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                                mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);
        }

        /* do the intranode reduce on all nodes other than the root's node */
        if (comm_ptr->node_comm != NULL &&
            MPIR_Get_intranode_rank(comm_ptr, root) == -1) {
            mpi_errno = MPID_Reduce(sendbuf, tmp_buf, count, datatype,
                                         op, 0, comm_ptr->node_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* do the internode reduce to the root's node */
        if (comm_ptr->node_roots_comm != NULL) {
            if (comm_ptr->node_roots_comm->rank != MPIR_Get_internode_rank(comm_ptr, root)) {
                /* I am not on root's node.  Use tmp_buf if we
                   participated in the first reduce, otherwise use sendbuf */
                const void *buf = (comm_ptr->node_comm == NULL ? sendbuf : tmp_buf);
                mpi_errno = MPID_Reduce(buf, NULL, count, datatype,
                                             op, MPIR_Get_internode_rank(comm_ptr, root),
                                             comm_ptr->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            else { /* I am on root's node. I have not participated in the earlier reduce. */
                if (comm_ptr->rank != root) {
                    /* I am not the root though. I don't have a valid recvbuf.
                       Use tmp_buf as recvbuf. */

                    mpi_errno = MPID_Reduce(sendbuf, tmp_buf, count, datatype,
                                                 op, MPIR_Get_internode_rank(comm_ptr, root),
                                                 comm_ptr->node_roots_comm, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }

                    /* point sendbuf at tmp_buf to make final intranode reduce easy */
                    sendbuf = tmp_buf;
                }
                else {
                    /* I am the root. in_place is automatically handled. */

                    mpi_errno = MPID_Reduce(sendbuf, recvbuf, count, datatype,
                                                 op, MPIR_Get_internode_rank(comm_ptr, root),
                                                 comm_ptr->node_roots_comm, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }

                    /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                    sendbuf = MPI_IN_PLACE;
                }
            }

        }

        /* do the intranode reduce on the root's node */
        if (comm_ptr->node_comm != NULL &&
            MPIR_Get_intranode_rank(comm_ptr, root) != -1) {
            mpi_errno = MPID_Reduce(sendbuf, recvbuf, count, datatype,
                                         op, MPIR_Get_intranode_rank(comm_ptr, root),
                                         comm_ptr->node_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        
        goto fn_exit;
    }
    }

    comm_size = comm_ptr->local_size;

    MPID_Datatype_get_size_macro(datatype, type_size);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    if ((count*type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        /* do a reduce-scatter followed by gather to root. */
        mpi_errno = MPIR_Reduce_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else {
        /* use a binomial tree algorithm */ 
        mpi_errno = MPIR_Reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
        

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



/* Needed in intercommunicator allreduce */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_inter ( 
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    int root,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
/*  Intercommunicator reduce.
    Remote group does a local intracommunicator
    reduce to rank 0. Rank 0 then sends data to root.

    Cost: (lgp+1).alpha + n.(lgp+1).beta
*/

    int rank, mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf=NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    if (root == MPI_ROOT) {
        /* root receives data from rank 0 on remote group */
        mpi_errno = MPIC_Recv(recvbuf, count, datatype, 0,
                                 MPIR_REDUCE_TAG, comm_ptr, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else {
        /* remote group. Rank 0 allocates temporary buffer, does
           local intracommunicator reduce, and then sends the data
           to root. */
        
        rank = comm_ptr->rank;
        
        if (rank == 0) {
            MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

            MPID_Datatype_get_extent_macro(datatype, extent);
	    /* I think this is the worse case, so we can avoid an assert() 
	     * inside the for loop */
	    /* Should MPIR_CHKLMEM_MALLOC do this? */
	    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
	    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "temporary buffer");
            /* adjust for potential negative lower bound in datatype */
            tmp_buf = (void *)((char*)tmp_buf - true_lb);
        }
        
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm) {
            mpi_errno = MPII_Setup_intercomm_localcomm( comm_ptr );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        newcomm_ptr = comm_ptr->local_comm;
        
        /* now do a local reduce on this intracommunicator */
        mpi_errno = MPIR_Reduce_intra(sendbuf, tmp_buf, count, datatype,
                                      op, 0, newcomm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        if (rank == 0)
	{
            mpi_errno = MPIC_Send(tmp_buf, count, datatype, root,
                                     MPIR_REDUCE_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


/* MPIR_Reduce performs an reduce using point-to-point messages.
   This is intended to be used by device-specific implementations of
   reduce.  In all other cases MPIR_Reduce_impl should be
   used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Reduce_intra(sendbuf, recvbuf, count, datatype,
                                      op, root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype,
                                      op, root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPIR_Reduce_impl should be called by any internal component that
   would otherwise call MPI_Reduce.  This differs from
   MPIR_Reduce in that this will call the coll_fns version if it
   exists.  This function replaces NMPI_Reduce. */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                     MPI_Op op, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Reduce != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Reduce(sendbuf, recvbuf, count,
                                               datatype, op, root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
            /* intracommunicator */
            mpi_errno = MPIR_Reduce_intra(sendbuf, recvbuf, count, datatype,
                                          op, root, comm_ptr, errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	} else {
            /* intercommunicator */
            mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype,
                                          op, root, comm_ptr, errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif


#undef FUNCNAME
#define FUNCNAME MPI_Reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@

MPI_Reduce - Reduces values on all processes to a single value

Input Parameters:
+ sendbuf - address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - reduce operation (handle) 
. root - rank of root process (integer) 
- comm - communicator (handle) 

Output Parameters:
. recvbuf - address of receive buffer (choice, 
 significant only at 'root') 

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_BUFFER_ALIAS

@*/
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	       MPI_Op op, int root, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_Datatype *datatype_ptr = NULL;
            MPIR_Op *op_ptr = NULL;
            int rank;
	    
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                MPIR_ERRTEST_COUNT(count, mpi_errno);
                MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
                if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype_get_ptr(datatype, datatype_ptr);
                    MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                }

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

	    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    MPIR_ERRTEST_COUNT(count, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
                    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(datatype, datatype_ptr);
                        MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);
                }
                
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(count, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
                    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(datatype, datatype_ptr);
                        MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);
                }
            }

	    MPIR_ERRTEST_OP(op, mpi_errno);

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr( op_ptr, mpi_errno );
            }
            if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = 
                    ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype); 
            }
	    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_reduce", "**mpi_reduce %p %p %d %D %O %d %C", sendbuf, recvbuf, 
				     count, datatype, op, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
