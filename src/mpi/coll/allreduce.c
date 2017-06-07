/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the short message algorithm will be used if the send buffer size is <=
        this value (in bytes)

    - name        : MPIR_CVAR_ENABLE_SMP_COLLECTIVES
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware collective communication.

    - name        : MPIR_CVAR_ENABLE_SMP_ALLREDUCE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware allreduce.

    - name        : MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware allreduce is used.  A
        value of '0' uses SMP-aware allreduce for all message sizes.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Allreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allreduce = PMPI_Allreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allreduce  MPI_Allreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allreduce as PMPI_Allreduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                  MPI_Op op, MPI_Comm comm)
                  __attribute__((weak,alias("PMPI_Allreduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allreduce
#define MPI_Allreduce PMPI_Allreduce

/* The order of entries in this table must match the definitions in 
   mpi.h.in */
MPI_User_function *MPIR_Op_table[] = { MPIR_MAXF, MPIR_MINF, MPIR_SUM,
                                       MPIR_PROD, MPIR_LAND,
                                       MPIR_BAND, MPIR_LOR, MPIR_BOR,
                                       MPIR_LXOR, MPIR_BXOR,
                                       MPIR_MINLOC, MPIR_MAXLOC, 
                                       MPIR_REPLACE, MPIR_NO_OP };

MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[] = {
    MPIR_MAXF_check_dtype, MPIR_MINF_check_dtype,
    MPIR_SUM_check_dtype,
    MPIR_PROD_check_dtype, MPIR_LAND_check_dtype,
    MPIR_BAND_check_dtype, MPIR_LOR_check_dtype, MPIR_BOR_check_dtype,
    MPIR_LXOR_check_dtype, MPIR_BXOR_check_dtype,
    MPIR_MINLOC_check_dtype, MPIR_MAXLOC_check_dtype,
    MPIR_REPLACE_check_dtype, MPIR_NO_OP_check_dtype }; 


/* This is the default implementation of allreduce. The algorithm is:
   
   Algorithm: MPI_Allreduce

   For the heterogeneous case, we call MPI_Reduce followed by MPI_Bcast
   in order to meet the requirement that all processes must have the
   same result. For the homogeneous case, we use the following algorithms.


   For long messages and for builtin ops and if count >= pof2 (where
   pof2 is the nearest power-of-two less than or equal to the number
   of processes), we use Rabenseifner's algorithm (see 
   http://www.hlrs.de/mpi/myreduce.html).
   This algorithm implements the allreduce in two steps: first a
   reduce-scatter, followed by an allgather. A recursive-halving
   algorithm (beginning with processes that are distance 1 apart) is
   used for the reduce-scatter, and a recursive doubling 
   algorithm is used for the allgather. The non-power-of-two case is
   handled by dropping to the nearest lower power-of-two: the first
   few even-numbered processes send their data to their right neighbors
   (rank+1), and the reduce-scatter and allgather happen among the remaining
   power-of-two processes. At the end, the first few even-numbered
   processes get the result from their right neighbors.

   For the power-of-two case, the cost for the reduce-scatter is 
   lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma. The cost for the
   allgather lgp.alpha + n.((p-1)/p).beta. Therefore, the
   total cost is:
   Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma

   For the non-power-of-two case, 
   Cost = (2.floor(lgp)+2).alpha + (2.((p-1)/p) + 2).n.beta + n.(1+(p-1)/p).gamma

   
   For short messages, for user-defined ops, and for count < pof2 
   we use a recursive doubling algorithm (similar to the one in
   MPI_Allgather). We use this algorithm in the case of user-defined ops
   because in this case derived datatypes are allowed, and the user
   could pass basic datatypes on one process and derived on another as
   long as the type maps are the same. Breaking up derived datatypes
   to do the reduce-scatter is tricky. 

   Cost = lgp.alpha + n.lgp.beta + n.lgp.gamma

   Possible improvements: 

   End Algorithm: MPI_Allreduce
*/

#undef FUNCNAME
#define FUNCNAME allreduce_intra_or_coll_fn
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int allreduce_intra_or_coll_fn(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Allreduce != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
        
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_intra ( 
    const void *sendbuf,
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
#ifdef MPID_HAS_HETERO
    int is_homogeneous;
    int rc;
#endif
    int comm_size, rank;
    MPI_Aint type_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int nbytes = 0;
    int mask, dst, is_commutative, pof2, newrank, rem, newdst;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;
    int count_lhalf, count_rhalf, step, nsteps, wsize;
    int *rindex, *rcount, *sindex, *scount;

    MPIR_CHKLMEM_DECL(5);

    if (count == 0) goto fn_exit;

    is_commutative = MPIR_Op_is_commutative(op);

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_ALLREDUCE) {
    /* is the op commutative? We do SMP optimizations only if it is. */
    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE ? type_size*count : 0;
    if (MPIR_Comm_is_node_aware(comm_ptr) && is_commutative &&
        nbytes <= MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE) {
        /* on each node, do a reduce to the local root */ 
        if (comm_ptr->node_comm != NULL) {
            /* take care of the MPI_IN_PLACE case. For reduce, 
               MPI_IN_PLACE is specified only on the root; 
               for allreduce it is specified on all processes. */

            if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
                /* IN_PLACE and not root of reduce. Data supplied to this
                   allreduce is in recvbuf. Pass that as the sendbuf to reduce. */

                mpi_errno = MPID_Reduce(recvbuf, NULL, count, datatype, op, 0, comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else {
                mpi_errno = MPID_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        } else {
            /* only one process on the node. copy sendbuf to recvbuf */
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }

        /* now do an IN_PLACE allreduce among the local roots of all nodes */
        if (comm_ptr->node_roots_comm != NULL) {
            mpi_errno = allreduce_intra_or_coll_fn(MPI_IN_PLACE, recvbuf, count, datatype, op, comm_ptr->node_roots_comm,
                                                   errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* now broadcast the result among local processes */
        if (comm_ptr->node_comm != NULL) {
            mpi_errno = MPID_Bcast(recvbuf, count, datatype, 0, comm_ptr->node_comm, errflag);
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
    
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
    else
        is_homogeneous = 1;
#endif
    
#ifdef MPID_HAS_HETERO
    if (!is_homogeneous) {
        /* heterogeneous. To get the same result on all processes, we
           do a reduce to 0 and then broadcast. */
        mpi_errno = MPID_Reduce( sendbuf, recvbuf, count, datatype,
                                       op, 0, comm_ptr, errflag );
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno = MPID_Bcast( recvbuf, count, datatype, 0, comm_ptr, errflag );
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else 
#endif /* MPID_HAS_HETERO */
    {
        /* homogeneous */

        comm_size = comm_ptr->local_size;
        rank = comm_ptr->rank;

        is_commutative = MPIR_Op_is_commutative(op);

        /* need to allocate temporary buffer to store incoming data*/
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPID_Datatype_get_extent_macro(datatype, extent);

        MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "temporary buffer");
	
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
        
        /* copy local data into recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                       count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        MPID_Datatype_get_size_macro(datatype, type_size);

        /* Find nearest power-of-two less than or equal to comm_size */
        pof2 = 1;
        nsteps = -1;
        while (pof2 <= comm_size) {
            pof2 <<= 1;
            nsteps++;
        }
        pof2 >>= 1;

        rem = comm_size - pof2;

        /* If op is user-defined or count is less than pof2, use
        recursive doubling algorithm. Otherwise do a reduce-scatter
        followed by allgather. (If op is user-defined,
        derived datatypes are allowed and the user could pass basic
        datatypes on one process and derived on another as long as
        the type maps are the same. Breaking up derived
        datatypes to do the reduce-scatter is tricky, therefore
        using recursive doubling in that case.) */

        if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
            (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
            /*
             * Use recursive doubling
             */
            if (rank < 2 * rem) {
                if (rank % 2 == 0) { /* even */
                    mpi_errno = MPIC_Send(recvbuf, count,
                                          datatype, rank + 1,
                                          MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }

                    /* temporarily set the rank to -1 so that this
                       process does not pariticipate in recursive doubling */
                    newrank = -1;
                }
                else { /* odd */
                    mpi_errno = MPIC_Recv(tmp_buf, count,
                                          datatype, rank - 1,
                                          MPIR_ALLREDUCE_TAG, comm_ptr,
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
                    mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                    /* change the rank */
                    newrank = rank / 2;
                }
            }
            else  /* rank >= 2 * rem */
                newrank = rank - rem;

            if (newrank != -1) {
                mask = 0x1;
                while (mask < pof2) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

                    /* Send the most current data, which is in recvbuf.
                       Recv into tmp_buf */
                    mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype,
                                              dst, MPIR_ALLREDUCE_TAG, tmp_buf,
                                              count, datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm_ptr,
                                              MPI_STATUS_IGNORE, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }

                    /* tmp_buf contains data received in this step.
                       recvbuf contains data accumulated so far */

                    if (is_commutative  || (dst < rank)) {
                        /* op is commutative OR the order is already right */
                        mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                    else {
                        /* op is noncommutative and the order is not right */
                        mpi_errno = MPIR_Reduce_local_impl(recvbuf, tmp_buf, count, datatype, op);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                        /* copy result back into recvbuf */
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                                   recvbuf, count, datatype);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                    mask <<= 1;
                }
            }

            /* In the non-power-of-two case, all odd-numbered
            processes of rank < 2 * rem send the result to
            (rank - 1), the ranks who didn't participate above. */
            if (rank < 2 * rem) {
                if (rank % 2)  /* odd */
                    mpi_errno = MPIC_Send(recvbuf, count,
                                          datatype, rank - 1,
                                          MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
                else  /* even */
                    mpi_errno = MPIC_Recv(recvbuf, count,
                                          datatype, rank + 1,
                                          MPIR_ALLREDUCE_TAG, comm_ptr,
                                          MPI_STATUS_IGNORE, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
        else {
            /*
             * Use reduce-scatter followed by allgather
             */
            
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
             * 0 to 2^{\floor(\log_2 p)} - 1. These odd ranks do not participate in the
             * rest of the algorithm.
             */
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
                        /* Recv into the left half of the current window, send the right
                         * half of the window to the peer (perform reduce on the left
                         * half of the current window)
                         */
                        rcount[step] = wsize / 2;
                        scount[step] = wsize - rcount[step];
                        sindex[step] = rindex[step] + rcount[step];
                    } else {
                        /* Recv into the right half of the current window, send the left
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
             * Step 3. Allgather by the recursive doubling algorithm.
             * Each process has 1 / p' of the total reduction result:
             * rcount[nsteps - 1] elements in the rbuf[rindex[nsteps - 1], ...].
             * All exchanges are executed in reverse order relative
             * to recursive doubling (previous step).
             */

            if (newrank != -1) {
                step = nsteps - 1; /* step = ilog2(p') - 1 */

                for (mask = pof2 >> 1; mask > 0; mask >>= 1) {
                    newdst = newrank ^ mask;
                    /* find real rank of dest */
                    dst = (newdst < rem) ? newdst * 2 : newdst + rem;

                    /*
                     * Send rcount[step] elements from rbuf[rindex[step]...]
                     * Recv scount[step] elements to rbuf[sindex[step]...]
                     */
                    mpi_errno = MPIC_Sendrecv((char *)recvbuf + rindex[step] * extent,
                                              rcount[step], datatype, dst,
                                              MPIR_ALLREDUCE_TAG,
                                              (char *)recvbuf + sindex[step] * extent,
                                              scount[step], datatype, dst,
                                              MPIR_ALLREDUCE_TAG, comm_ptr,
                                              MPI_STATUS_IGNORE, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                    step--;
                }
            }
            /*
             * Step 4. Send total result to excluded odd ranks.
             */
            if (rank < 2 * rem) {
                if (rank % 2 != 0) {
                    /* Odd process -- recv result from rank - 1 */

                    mpi_errno = MPIC_Recv(recvbuf, count, datatype, rank - 1,
                                          MPIR_ALLREDUCE_TAG, comm_ptr,
                                          MPI_STATUS_IGNORE, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                } else {
                    /* Even process -- send result to rank + 1 */
                    mpi_errno = MPIC_Send(recvbuf, count, datatype, rank + 1,
                                          MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        } /* reduce-scatter followed by allgather */
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    return (mpi_errno);

  fn_fail:
    goto fn_exit;
}


/* not declared static because a machine-specific function may call this one 
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_inter ( 
    const void *sendbuf,
    void *recvbuf, 
    int count, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
/* Intercommunicator Allreduce.
   We first do intracommunicator reduces to rank 0 on left and right
   groups, then an exchange between left and right rank 0, and finally
   intracommunicator broadcasts from rank 0 on left and right group.
*/
    int mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf=NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (comm_ptr->rank == 0) {
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
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    /* Do a local reduce on this intracommunicator */
    mpi_errno = MPIR_Reduce_intra(sendbuf, tmp_buf, count, datatype,
                                  op, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* Do a exchange between local and remote rank 0 on this intercommunicator */
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(tmp_buf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  recvbuf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Do a local broadcast on this intracommunicator */
    mpi_errno = MPID_Bcast(recvbuf, count, datatype,
                                0, newcomm_ptr, errflag);
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

/* MPIR_Allreduce performs an allreduce using point-to-point messages.
   This is intended to be used by device-specific implementations of
   allreduce.  In all other cases MPIR_Allreduce_impl should be
   used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* intercommunicator */
        mpi_errno = MPIR_Allreduce_inter(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:

    goto fn_exit;
}

/* MPIR_Allreduce_impl should be called by any internal component that
   would otherwise call MPI_Allreduce.  This differs from
   MPIR_Allreduce in that this will call the coll_fns version if it
   exists.  */
#undef FUNCNAME
#define FUNCNAME MPIR_Allreduce_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allreduce_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr,
                        MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Allreduce != NULL)
    {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    }
    else
    {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
            /* intracommunicator */
            mpi_errno = MPIR_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	}
        else {
            /* intercommunicator */
            mpi_errno = MPIR_Allreduce_inter(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
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
#define FUNCNAME MPI_Allreduce
#undef FCNAME

/*@
MPI_Allreduce - Combines values from all processes and distributes the result
                back to all processes

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameters:
. recvbuf - starting address of receive buffer (choice) 

.N ThreadSafe

.N Fortran

.N collops

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_OP
.N MPI_ERR_COMM
@*/
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Allreduce";
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLREDUCE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLREDUCE);

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

            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    MPIR_ERRTEST_OP(op, mpi_errno);
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);
            } else {
                if (count != 0 && sendbuf != MPI_IN_PLACE)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            }
            
            if (sendbuf != MPI_IN_PLACE) 
                MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, count, mpi_errno);
	    MPIR_ERRTEST_USERBUFFER(recvbuf,count,datatype,mpi_errno);

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

    mpi_errno = MPID_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLREDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_allreduce",
	    "**mpi_allreduce %p %p %d %D %O %C", sendbuf, recvbuf, count, datatype, op, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
