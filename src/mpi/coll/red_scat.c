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
    - name        : MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the long message algorithm will be used if the operation is commutative
        and the send buffer size is >= this value (in bytes)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Reduce_scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Reduce_scatter = PMPI_Reduce_scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Reduce_scatter  MPI_Reduce_scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Reduce_scatter as PMPI_Reduce_scatter
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
                       __attribute__((weak,alias("PMPI_Reduce_scatter")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Reduce_scatter
#define MPI_Reduce_scatter PMPI_Reduce_scatter

/* Implements the reduce-scatter butterfly algorithm described in J. L. Traff's
 * "An Improved Algorithm for (Non-commutative) Reduce-Scatter with an 
 * Application"
 * from EuroPVM/MPI 2005.  This function currently only implements support for
 * the power-of-2, block-regular case (all receive counts are equal). */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_noncomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIR_Reduce_scatter_noncomm(const void *sendbuf, void *recvbuf, const int recvcounts[],
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                                       MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    int pof2;
    int log2_comm_size;
    int i, k;
    int recv_offset, send_offset;
    int block_size, total_count, size;
    MPI_Aint true_extent, true_lb;
    int buf0_was_inout;
    void *tmp_buf0;
    void *tmp_buf1;
    void *result_ptr;
    MPIR_CHKLMEM_DECL(3);

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    pof2 = 1;
    log2_comm_size = 0;
    while (pof2 < comm_size) {
        pof2 <<= 1;
        ++log2_comm_size;
    }

    /* begin error checking */
    MPIR_Assert(pof2 == comm_size); /* FIXME this version only works for power of 2 procs */

    for (i = 0; i < (comm_size - 1); ++i) {
        MPIR_Assert(recvcounts[i] == recvcounts[i+1]);
    }
    /* end error checking */

    /* size of a block (count of datatype per block, NOT bytes per block) */
    block_size = recvcounts[0];
    total_count = block_size * comm_size;

    MPIR_CHKLMEM_MALLOC(tmp_buf0, void *, true_extent * total_count, mpi_errno, "tmp_buf0");
    MPIR_CHKLMEM_MALLOC(tmp_buf1, void *, true_extent * total_count, mpi_errno, "tmp_buf1");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf0 = (void *)((char*)tmp_buf0 - true_lb);
    tmp_buf1 = (void *)((char*)tmp_buf1 - true_lb);

    /* Copy our send data to tmp_buf0.  We do this one block at a time and
       permute the blocks as we go according to the mirror permutation. */
    for (i = 0; i < comm_size; ++i) {
        mpi_errno = MPIR_Localcopy((char *)(sendbuf == MPI_IN_PLACE ? (const void *)recvbuf : sendbuf) + (i * true_extent * block_size),
                                   block_size, datatype,
                                   (char *)tmp_buf0 + (MPIU_Mirror_permutation(i, log2_comm_size) * true_extent * block_size), block_size, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    buf0_was_inout = 1;

    send_offset = 0;
    recv_offset = 0;
    size = total_count;
    for (k = 0; k < log2_comm_size; ++k) {
        /* use a double-buffering scheme to avoid local copies */
        char *incoming_data = (buf0_was_inout ? tmp_buf1 : tmp_buf0);
        char *outgoing_data = (buf0_was_inout ? tmp_buf0 : tmp_buf1);
        int peer = rank ^ (0x1 << k);
        size /= 2;

        if (rank > peer) {
            /* we have the higher rank: send top half, recv bottom half */
            recv_offset += size;
        }
        else {
            /* we have the lower rank: recv top half, send bottom half */
            send_offset += size;
        }

        mpi_errno = MPIC_Sendrecv(outgoing_data + send_offset*true_extent,
                                     size, datatype, peer, MPIR_REDUCE_SCATTER_TAG,
                                     incoming_data + recv_offset*true_extent,
                                     size, datatype, peer, MPIR_REDUCE_SCATTER_TAG,
                                     comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        /* always perform the reduction at recv_offset, the data at send_offset
           is now our peer's responsibility */
        if (rank > peer) {
            /* higher ranked value so need to call op(received_data, my_data) */
	    mpi_errno = MPIR_Reduce_local_impl( 
		     incoming_data + recv_offset*true_extent,
                     outgoing_data + recv_offset*true_extent,
                     size, datatype, op );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else {
            /* lower ranked value so need to call op(my_data, received_data) */
	    MPIR_Reduce_local_impl( 
		     outgoing_data + recv_offset*true_extent,
                     incoming_data + recv_offset*true_extent,
                     size, datatype, op);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            buf0_was_inout = !buf0_was_inout;
        }

        /* the next round of send/recv needs to happen within the block (of size
           "size") that we just received and reduced */
        send_offset = recv_offset;
    }

    MPIR_Assert(size == recvcounts[rank]);

    /* copy the reduced data to the recvbuf */
    result_ptr = (char *)(buf0_was_inout ? tmp_buf0 : tmp_buf1) + recv_offset * true_extent;
    mpi_errno = MPIR_Localcopy(result_ptr, size, datatype,
                               recvbuf, size, datatype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

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

/* This is the default implementation of reduce_scatter. The algorithm is:

   Algorithm: MPI_Reduce_scatter

   If the operation is commutative, for short and medium-size
   messages, we use a recursive-halving
   algorithm in which the first p/2 processes send the second n/2 data
   to their counterparts in the other half and receive the first n/2
   data from them. This procedure continues recursively, halving the
   data communicated at each step, for a total of lgp steps. If the
   number of processes is not a power-of-two, we convert it to the
   nearest lower power-of-two by having the first few even-numbered
   processes send their data to the neighboring odd-numbered process
   at (rank+1). Those odd-numbered processes compute the result for
   their left neighbor as well in the recursive halving algorithm, and
   then at  the end send the result back to the processes that didn't
   participate.
   Therefore, if p is a power-of-two,
   Cost = lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
   If p is not a power-of-two,
   Cost = (floor(lgp)+2).alpha + n.(1+(p-1+n)/p).beta + n.(1+(p-1)/p).gamma
   The above cost in the non power-of-two case is approximate because
   there is some imbalance in the amount of work each process does
   because some processes do the work of their neighbors as well.

   For commutative operations and very long messages we use
   we use a pairwise exchange algorithm similar to
   the one used in MPI_Alltoall. At step i, each process sends n/p
   amount of data to (rank+i) and receives n/p amount of data from
   (rank-i).
   Cost = (p-1).alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma


   If the operation is not commutative, we do the following:

   We use a recursive doubling algorithm, which
   takes lgp steps. At step 1, processes exchange (n-n/p) amount of
   data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
   amount of data, and so forth.

   Cost = lgp.alpha + n.(lgp-(p-1)/p).beta + n.(lgp-(p-1)/p).gamma

   Possible improvements:

   End Algorithm: MPI_Reduce_scatter
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Reduce_scatter_intra(const void *sendbuf, void *recvbuf, const int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int   rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb; 
    int  *disps;
    void *tmp_recvbuf, *tmp_results;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int type_size, dis[2], blklens[2], total_count, nbytes, src, dst;
    int mask, dst_tree_root, my_tree_root, j, k;
    int *newcnts, *newdisps, rem, newdst, send_idx, recv_idx,
        last_idx, send_cnt, recv_cnt;
    int pof2, old_i, newrank, received;
    MPI_Datatype sendtype, recvtype;
    int nprocs_completed, tmp_mask, tree_root, is_commutative;
    MPIR_Op *op_ptr;
    MPIR_CHKLMEM_DECL(5);

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

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    
    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
    }
    else {
        MPIR_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            is_commutative = 0;
        else
            is_commutative = 1;
    }

    MPIR_CHKLMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps");

    total_count = 0;
    for (i=0; i<comm_size; i++) {
        disps[i] = total_count;
        total_count += recvcounts[i];
    }
    
    if (total_count == 0) {
        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;
    
    /* total_count*extent eventually gets malloced. it isn't added to
     * a user-passed in buffer */
    MPIR_Ensure_Aint_fits_in_pointer(total_count * MPL_MAX(true_extent, extent));

    if ((is_commutative) && (nbytes < MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and short. use recursive halving algorithm */

        /* allocate temp. buffer to receive incoming data */
        MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_recvbuf");
        /* adjust for potential negative lower bound in datatype */
        tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);
            
        /* need to allocate another temporary buffer to accumulate
           results because recvbuf may not be big enough */
        MPIR_CHKLMEM_MALLOC(tmp_results, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_results");
        /* adjust for potential negative lower bound in datatype */
        tmp_results = (void *)((char*)tmp_results - true_lb);
        
        /* copy sendbuf into tmp_results */
        if (sendbuf != MPI_IN_PLACE)
            mpi_errno = MPIR_Localcopy(sendbuf, total_count, datatype,
                                       tmp_results, total_count, datatype);
        else
            mpi_errno = MPIR_Localcopy(recvbuf, total_count, datatype,
                                       tmp_results, total_count, datatype);
        
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        pof2 = 1;
        while (pof2 <= comm_size) pof2 <<= 1;
        pof2 >>=1;

        rem = comm_size - pof2;

        /* In the non-power-of-two case, all even-numbered
           processes of rank < 2*rem send their data to
           (rank+1). These even-numbered processes no longer
           participate in the algorithm until the very end. The
           remaining processes form a nice power-of-two. */

        if (rank < 2*rem) {
            if (rank % 2 == 0) { /* even */
                mpi_errno = MPIC_Send(tmp_results, total_count,
                                         datatype, rank+1,
                                         MPIR_REDUCE_SCATTER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                
                /* temporarily set the rank to -1 so that this
                   process does not pariticipate in recursive
                   doubling */
                newrank = -1; 
            }
            else { /* odd */
                mpi_errno = MPIC_Recv(tmp_recvbuf, total_count,
                                         datatype, rank-1,
                                         MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                         MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                
                /* do the reduction on received data. since the
                   ordering is right, it doesn't matter whether
                   the operation is commutative or not. */
		mpi_errno = MPIR_Reduce_local_impl( 
		    tmp_recvbuf, tmp_results, total_count, datatype, op );
                
                /* change the rank */
                newrank = rank / 2;
            }
        }
        else  /* rank >= 2*rem */
            newrank = rank - rem;

        if (newrank != -1) {
            /* recalculate the recvcounts and disps arrays because the
               even-numbered processes who no longer participate will
               have their result calculated by the process to their
               right (rank+1). */

            MPIR_CHKLMEM_MALLOC(newcnts, int *, pof2*sizeof(int), mpi_errno, "newcnts");
            MPIR_CHKLMEM_MALLOC(newdisps, int *, pof2*sizeof(int), mpi_errno, "newdisps");
            
            for (i=0; i<pof2; i++) {
                /* what does i map to in the old ranking? */
                old_i = (i < rem) ? i*2 + 1 : i + rem;
                if (old_i < 2*rem) {
                    /* This process has to also do its left neighbor's
                       work */
                    newcnts[i] = recvcounts[old_i] + recvcounts[old_i-1];
                }
                else
                    newcnts[i] = recvcounts[old_i];
            }
            
            newdisps[0] = 0;
            for (i=1; i<pof2; i++)
                newdisps[i] = newdisps[i-1] + newcnts[i-1];

            mask = pof2 >> 1;
            send_idx = recv_idx = 0;
            last_idx = pof2;
            while (mask > 0) {
                newdst = newrank ^ mask;
                /* find real rank of dest */
                dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;
                
                send_cnt = recv_cnt = 0;
                if (newrank < newdst) {
                    send_idx = recv_idx + mask;
                    for (i=send_idx; i<last_idx; i++)
                        send_cnt += newcnts[i];
                    for (i=recv_idx; i<send_idx; i++)
                        recv_cnt += newcnts[i];
                }
                else {
                    recv_idx = send_idx + mask;
                    for (i=send_idx; i<recv_idx; i++)
                        send_cnt += newcnts[i];
                    for (i=recv_idx; i<last_idx; i++)
                        recv_cnt += newcnts[i];
                }
                
/*                    printf("Rank %d, send_idx %d, recv_idx %d, send_cnt %d, recv_cnt %d, last_idx %d\n", newrank, send_idx, recv_idx,
                      send_cnt, recv_cnt, last_idx);
*/
                /* Send data from tmp_results. Recv into tmp_recvbuf */ 
                if ((send_cnt != 0) && (recv_cnt != 0)) 
                    mpi_errno = MPIC_Sendrecv((char *) tmp_results +
                                                 newdisps[send_idx]*extent,
                                                 send_cnt, datatype,
                                                 dst, MPIR_REDUCE_SCATTER_TAG,
                                                 (char *) tmp_recvbuf +
                                                 newdisps[recv_idx]*extent,
                                                 recv_cnt, datatype, dst,
                                                 MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                                 MPI_STATUS_IGNORE, errflag);
                else if ((send_cnt == 0) && (recv_cnt != 0))
                    mpi_errno = MPIC_Recv((char *) tmp_recvbuf +
                                             newdisps[recv_idx]*extent,
                                             recv_cnt, datatype, dst,
                                             MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
                else if ((recv_cnt == 0) && (send_cnt != 0))
                    mpi_errno = MPIC_Send((char *) tmp_results +
                                             newdisps[send_idx]*extent,
                                             send_cnt, datatype,
                                             dst, MPIR_REDUCE_SCATTER_TAG,
                                             comm_ptr, errflag);

                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                
                /* tmp_recvbuf contains data received in this step.
                   tmp_results contains data accumulated so far */
                
                if (recv_cnt) {
		    mpi_errno = MPIR_Reduce_local_impl( 
			     (char *) tmp_recvbuf + newdisps[recv_idx]*extent,
                             (char *) tmp_results + newdisps[recv_idx]*extent, 
			     recv_cnt, datatype, op);
                }

                /* update send_idx for next iteration */
                send_idx = recv_idx;
                last_idx = recv_idx + mask;
                mask >>= 1;
            }

            /* copy this process's result from tmp_results to recvbuf */
            if (recvcounts[rank]) {
                mpi_errno = MPIR_Localcopy((char *)tmp_results +
                                           disps[rank]*extent, 
                                           recvcounts[rank], datatype, recvbuf,
                                           recvcounts[rank], datatype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            
        }

        /* In the non-power-of-two case, all odd-numbered
           processes of rank < 2*rem send to (rank-1) the result they
           calculated for that process */
        if (rank < 2*rem) {
            if (rank % 2) { /* odd */
                if (recvcounts[rank-1]) {
                    mpi_errno = MPIC_Send((char *) tmp_results +
                                             disps[rank-1]*extent, recvcounts[rank-1],
                                             datatype, rank-1,
                                             MPIR_REDUCE_SCATTER_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
            else  {   /* even */
                if (recvcounts[rank]) {
                    mpi_errno = MPIC_Recv(recvbuf, recvcounts[rank],
                                             datatype, rank+1,
                                             MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
    }
    
    if (is_commutative && (nbytes >= MPIR_CVAR_REDSCAT_COMMUTATIVE_LONG_MSG_SIZE)) {

        /* commutative and long message, or noncommutative and long message.
           use (p-1) pairwise exchanges */ 

        if (sendbuf != MPI_IN_PLACE) {
            /* copy local data into recvbuf */
            mpi_errno = MPIR_Localcopy(((char *)sendbuf+disps[rank]*extent),
                                       recvcounts[rank], datatype, recvbuf,
                                       recvcounts[rank], datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        
        /* allocate temporary buffer to store incoming data */
        MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, recvcounts[rank]*(MPL_MAX(true_extent,extent))+1, mpi_errno, "tmp_recvbuf");
        /* adjust for potential negative lower bound in datatype */
        tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);
        
        for (i=1; i<comm_size; i++) {
            src = (rank - i + comm_size) % comm_size;
            dst = (rank + i) % comm_size;
            
            /* send the data that dst needs. recv data that this process
               needs from src into tmp_recvbuf */
            if (sendbuf != MPI_IN_PLACE) 
                mpi_errno = MPIC_Sendrecv(((char *)sendbuf+disps[dst]*extent),
                                             recvcounts[dst], datatype, dst,
                                             MPIR_REDUCE_SCATTER_TAG, tmp_recvbuf,
                                             recvcounts[rank], datatype, src,
                                             MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
            else
                mpi_errno = MPIC_Sendrecv(((char *)recvbuf+disps[dst]*extent),
                                             recvcounts[dst], datatype, dst,
                                             MPIR_REDUCE_SCATTER_TAG, tmp_recvbuf,
                                             recvcounts[rank], datatype, src,
                                             MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                             MPI_STATUS_IGNORE, errflag);
            
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            
            if (is_commutative || (src < rank)) {
                if (sendbuf != MPI_IN_PLACE) {
		    mpi_errno = MPIR_Reduce_local_impl( 
			       tmp_recvbuf, recvbuf, recvcounts[rank],
                               datatype, op ); 
                }
                else {
		    mpi_errno = MPIR_Reduce_local_impl( 
			tmp_recvbuf, ((char *)recvbuf+disps[rank]*extent), 
			recvcounts[rank], datatype, op);
                    /* we can't store the result at the beginning of
                       recvbuf right here because there is useful data
                       there that other process/processes need. at the
                       end, we will copy back the result to the
                       beginning of recvbuf. */
                }
            }
            else {
                if (sendbuf != MPI_IN_PLACE) {
		    mpi_errno = MPIR_Reduce_local_impl( 
		       recvbuf, tmp_recvbuf, recvcounts[rank], datatype, op);
                    /* copy result back into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_recvbuf, recvcounts[rank],
                                               datatype, recvbuf,
                                               recvcounts[rank], datatype);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
		    mpi_errno = MPIR_Reduce_local_impl( 
                        ((char *)recvbuf+disps[rank]*extent),
			tmp_recvbuf, recvcounts[rank], datatype, op);
                    /* copy result back into recvbuf */
                    mpi_errno = MPIR_Localcopy(tmp_recvbuf, recvcounts[rank],
                                               datatype, 
                                               ((char *)recvbuf +
                                                disps[rank]*extent), 
                                               recvcounts[rank], datatype);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        
        /* if MPI_IN_PLACE, move output data to the beginning of
           recvbuf. already done for rank 0. */
        if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
            mpi_errno = MPIR_Localcopy(((char *)recvbuf +
                                        disps[rank]*extent),  
                                       recvcounts[rank], datatype,
                                       recvbuf, 
                                       recvcounts[rank], datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }
    
    if (!is_commutative) {
        int is_block_regular = 1;
        for (i = 0; i < (comm_size - 1); ++i) {
            if (recvcounts[i] != recvcounts[i+1]) {
                is_block_regular = 0;
                break;
            }
        }

        /* slightly retask pof2 to mean pof2 equal or greater, not always greater as it is above */
        pof2 = 1;
        while (pof2 < comm_size) pof2 <<= 1;

        if (pof2 == comm_size && is_block_regular) {
            /* noncommutative, pof2 size, and block regular */
            mpi_errno = MPIR_Reduce_scatter_noncomm(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */

            /* need to allocate temporary buffer to receive incoming data*/
            MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_recvbuf");
            /* adjust for potential negative lower bound in datatype */
            tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);

            /* need to allocate another temporary buffer to accumulate
               results */
            MPIR_CHKLMEM_MALLOC(tmp_results, void *, total_count*(MPL_MAX(true_extent,extent)), mpi_errno, "tmp_results");
            /* adjust for potential negative lower bound in datatype */
            tmp_results = (void *)((char*)tmp_results - true_lb);

            /* copy sendbuf into tmp_results */
            if (sendbuf != MPI_IN_PLACE)
                mpi_errno = MPIR_Localcopy(sendbuf, total_count, datatype,
                                           tmp_results, total_count, datatype);
            else
                mpi_errno = MPIR_Localcopy(recvbuf, total_count, datatype,
                                           tmp_results, total_count, datatype);

            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            mask = 0x1;
            i = 0;
            while (mask < comm_size) {
                dst = rank ^ mask;

                dst_tree_root = dst >> i;
                dst_tree_root <<= i;

                my_tree_root = rank >> i;
                my_tree_root <<= i;

                /* At step 1, processes exchange (n-n/p) amount of
                   data; at step 2, (n-2n/p) amount of data; at step 3, (n-4n/p)
                   amount of data, and so forth. We use derived datatypes for this.

                   At each step, a process does not need to send data
                   indexed from my_tree_root to
                   my_tree_root+mask-1. Similarly, a process won't receive
                   data indexed from dst_tree_root to dst_tree_root+mask-1. */

                /* calculate sendtype */
                blklens[0] = blklens[1] = 0;
                for (j=0; j<my_tree_root; j++)
                    blklens[0] += recvcounts[j];
                for (j=my_tree_root+mask; j<comm_size; j++)
                    blklens[1] += recvcounts[j];

                dis[0] = 0;
                dis[1] = blklens[0];
                for (j=my_tree_root; (j<my_tree_root+mask) && (j<comm_size); j++)
                    dis[1] += recvcounts[j];

                mpi_errno = MPIR_Type_indexed_impl(2, blklens, dis, datatype, &sendtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                
                mpi_errno = MPIR_Type_commit_impl(&sendtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                /* calculate recvtype */
                blklens[0] = blklens[1] = 0;
                for (j=0; j<dst_tree_root && j<comm_size; j++)
                    blklens[0] += recvcounts[j];
                for (j=dst_tree_root+mask; j<comm_size; j++)
                    blklens[1] += recvcounts[j];

                dis[0] = 0;
                dis[1] = blklens[0];
                for (j=dst_tree_root; (j<dst_tree_root+mask) && (j<comm_size); j++)
                    dis[1] += recvcounts[j];

                mpi_errno = MPIR_Type_indexed_impl(2, blklens, dis, datatype, &recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                
                mpi_errno = MPIR_Type_commit_impl(&recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                received = 0;
                if (dst < comm_size) {
                    /* tmp_results contains data to be sent in each step. Data is
                       received in tmp_recvbuf and then accumulated into
                       tmp_results. accumulation is done later below.   */ 

                    mpi_errno = MPIC_Sendrecv(tmp_results, 1, sendtype, dst,
                                                 MPIR_REDUCE_SCATTER_TAG, 
                                                 tmp_recvbuf, 1, recvtype, dst,
                                                 MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                                 MPI_STATUS_IGNORE, errflag);
                    received = 1;
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }

                /* if some processes in this process's subtree in this step
                   did not have any destination process to communicate with
                   because of non-power-of-two, we need to send them the
                   result. We use a logarithmic recursive-halfing algorithm
                   for this. */

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
                            && (dst >= tree_root + nprocs_completed)) {
                            /* send the current result */
                            mpi_errno = MPIC_Send(tmp_recvbuf, 1, recvtype,
                                                     dst, MPIR_REDUCE_SCATTER_TAG,
                                                     comm_ptr, errflag);
                            if (mpi_errno) {
                                /* for communication errors, just record the error but continue */
                                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                            }
                        }
                        /* recv only if this proc. doesn't have data and sender
                           has data */
                        else if ((dst < rank) && 
                                 (dst < tree_root + nprocs_completed) &&
                                 (rank >= tree_root + nprocs_completed)) {
                            mpi_errno = MPIC_Recv(tmp_recvbuf, 1, recvtype, dst,
                                                     MPIR_REDUCE_SCATTER_TAG,
                                                     comm_ptr, MPI_STATUS_IGNORE, errflag);
                            received = 1;
                            if (mpi_errno) {
                                /* for communication errors, just record the error but continue */
                                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                            }
                        }
                        tmp_mask >>= 1;
                        k--;
                    }
                }

                /* The following reduction is done here instead of after 
                   the MPIC_Sendrecv or MPIC_Recv above. This is
                   because to do it above, in the noncommutative 
                   case, we would need an extra temp buffer so as not to
                   overwrite temp_recvbuf, because temp_recvbuf may have
                   to be communicated to other processes in the
                   non-power-of-two case. To avoid that extra allocation,
                   we do the reduce here. */
                if (received) {
                    if (is_commutative || (dst_tree_root < my_tree_root)) {
                        {
			    mpi_errno = MPIR_Reduce_local_impl( 
                               tmp_recvbuf, tmp_results, blklens[0],
			       datatype, op); 
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

			    mpi_errno = MPIR_Reduce_local_impl( 
                               ((char *)tmp_recvbuf + dis[1]*extent),
			       ((char *)tmp_results + dis[1]*extent),
			       blklens[1], datatype, op); 
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        }
                    }
                    else {
                        {
			    mpi_errno = MPIR_Reduce_local_impl(
                                   tmp_results, tmp_recvbuf, blklens[0],
                                   datatype, op); 
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

			    mpi_errno = MPIR_Reduce_local_impl(
                                   ((char *)tmp_results + dis[1]*extent),
                                   ((char *)tmp_recvbuf + dis[1]*extent),
                                   blklens[1], datatype, op); 
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        }
                        /* copy result back into tmp_results */
                        mpi_errno = MPIR_Localcopy(tmp_recvbuf, 1, recvtype, 
                                                   tmp_results, 1, recvtype);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }

                MPIR_Type_free_impl(&sendtype);
                MPIR_Type_free_impl(&recvtype);

                mask <<= 1;
                i++;
            }

            /* now copy final results from tmp_results to recvbuf */
            mpi_errno = MPIR_Localcopy(((char *)tmp_results+disps[rank]*extent),
                                       recvcounts[rank], datatype, recvbuf,
                                       recvcounts[rank], datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno)
            mpi_errno = per_thread->op_errno;
    }

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Reduce_scatter_inter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                              MPIR_Errflag_t *errflag)
{
/* Intercommunicator Reduce_scatter.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator scattervs in each group.
*/
    
    int rank, mpi_errno, root, local_size, total_count, i;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb = 0, extent;
    void *tmp_buf=NULL;
    int *disps=NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(2);

    rank = comm_ptr->rank;
    local_size = comm_ptr->local_size;

    total_count = 0;
    for (i=0; i<local_size; i++) total_count += recvcounts[i];

    if (rank == 0) {
        /* In each group, rank 0 allocates a temp. buffer for the 
           reduce */
        
        MPIR_CHKLMEM_MALLOC(disps, int *, local_size*sizeof(int), mpi_errno, "disps");

        total_count = 0;
        for (i=0; i<local_size; i++) {
            disps[i] = total_count;
            total_count += recvcounts[i];
        }

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, total_count*(MPL_MAX(extent,true_extent)), mpi_errno, "tmp_buf");

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce_inter(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Reduce_inter(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Reduce_inter(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Reduce_inter(sendbuf, tmp_buf, total_count, datatype, op,
                                root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
	mpi_errno = MPII_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    newcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPIR_Scatterv(tmp_buf, recvcounts, disps, datatype, recvbuf,
                              recvcounts[rank], datatype, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
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


/* MPIR_Reduce_Scatter performs an reduce_scatter using point-to-point
   messages.  This is intended to be used by device-specific
   implementations of reduce_scatter.  In all other cases
   MPIR_Reduce_Scatter_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Reduce_scatter_intra(sendbuf, recvbuf, recvcounts,
                                              datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Reduce_scatter_inter(sendbuf, recvbuf, recvcounts,
                                              datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* MPIR_Reduce_Scatter_impl should be called by any internal component
   that would otherwise call MPI_Reduce_Scatter.  This differs from
   MPIR_Reduce_Scatter in that this will call the coll_fns version if
   it exists.  This function replaces NMPI_Reduce_Scatter. */
#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_impl(const void *sendbuf, void *recvbuf, const int recvcounts[],
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->coll_fns != NULL && 
	comm_ptr->coll_fns->Reduce_scatter != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Reduce_scatter(sendbuf, recvbuf, recvcounts,
                                                       datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Reduce_scatter(sendbuf, recvbuf, recvcounts,
                                        datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Reduce_scatter - Combines values and scatters the results

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. recvcounts - integer array specifying the 
number of elements in result distributed to each process.
Array must be identical on all calling processes. 
. datatype - data type of elements of input buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameters:
. recvbuf - starting address of receive buffer (choice) 

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_OP
.N MPI_ERR_BUFFER_ALIAS
@*/
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[],
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_REDUCE_SCATTER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_REDUCE_SCATTER);

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
            int i, size, sum;
	    
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            size = comm_ptr->local_size; 
            /* even in intercomm. case, recvcounts is of size local_size */

            sum = 0;
	    for (i=0; i<size; i++) {
		MPIR_ERRTEST_COUNT(recvcounts[i],mpi_errno);
                sum += recvcounts[i];
	    }

	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[comm_ptr->rank], mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sum, mpi_errno);
            } else if (sendbuf != MPI_IN_PLACE && sum != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno)

            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[comm_ptr->rank],datatype,mpi_errno);
            MPIR_ERRTEST_USERBUFFER(sendbuf,sum,datatype,mpi_errno); 
	    MPIR_ERRTEST_OP(op, mpi_errno);

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

    mpi_errno = MPID_Reduce_scatter(sendbuf, recvbuf, recvcounts,
                                    datatype, op, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_REDUCE_SCATTER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_reduce_scatter",
	    "**mpi_reduce_scatter %p %p %p %D %O %C", sendbuf, recvbuf, recvcounts, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
