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
    - name        : MPIR_CVAR_USE_BCAST
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Variable to select bcast algorithm
        0 - MPIR_Bcast_intra
        1 - MPIR_MPICH_TREE_Bcast with knomial trees
        2 - MPIR_MPICH_TREE_Bcast with kary trees

    - name        : MPIR_CVAR_BCAST_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based bcast - MPIR_MPICH_TREE_Bcast 

    - name        : MPIR_CVAR_BCAST_MIN_PROCS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_SHORT_MSG_SIZE, MPIR_CVAR_BCAST_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_BCAST_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 12288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_MIN_PROCS, MPIR_CVAR_BCAST_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_BCAST_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Let's define short messages as messages with size < MPIR_CVAR_BCAST_SHORT_MSG_SIZE,
        and medium messages as messages with size >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE but
        < MPIR_CVAR_BCAST_LONG_MSG_SIZE, and long messages as messages with size >=
        MPIR_CVAR_BCAST_LONG_MSG_SIZE. The broadcast algorithms selection procedure is
        as follows. For short messages or when the number of processes is <
        MPIR_CVAR_BCAST_MIN_PROCS, we do broadcast using the binomial tree algorithm.
        Otherwise, for medium messages and with a power-of-two number of processes, we do
        broadcast based on a scatter followed by a recursive doubling allgather algorithm.
        Otherwise, for long messages or with non power-of-two number of processes, we do
        broadcast based on a scatter followed by a ring allgather algorithm.
        (See also: MPIR_CVAR_BCAST_MIN_PROCS, MPIR_CVAR_BCAST_SHORT_MSG_SIZE)

    - name        : MPIR_CVAR_ENABLE_SMP_BCAST
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : Enable SMP aware broadcast (See also: MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE)

    - name        : MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware broadcast is used.  A
        value of '0' uses SMP-aware broadcast for all message sizes.
        (See also: MPIR_CVAR_ENABLE_SMP_BCAST)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

enum {
    MPIR_BCAST_INTRA=0,
    MPIR_MPICH_TREE_BCAST_KNOMIAL,
    MPIR_MPICH_TREE_BCAST_KARY
};

/* -- Begin Profiling Symbol Block for routine MPI_Bcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bcast = PMPI_Bcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bcast  MPI_Bcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bcast as PMPI_Bcast
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
              __attribute__((weak,alias("PMPI_Bcast")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Bcast
#define MPI_Bcast PMPI_Bcast

/* FIXME This function uses some heuristsics based off of some testing on a
 * cluster at Argonne.  We need a better system for detrmining and controlling
 * the cutoff points for these algorithms.  If I've done this right, you should
 * be able to make changes along these lines almost exclusively in this function
 * and some new functions. [goodell@ 2008/01/07] */
#undef FUNCNAME
#define FUNCNAME smp_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int smp_bcast(
        void *buffer, 
        int count, 
        MPI_Datatype datatype, 
        int root, 
        MPIR_Comm *comm_ptr,
        MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int is_homogeneous;
    MPI_Aint type_size, nbytes=0;
    MPI_Status status;
    int recvd_size;

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_BCAST) {
        MPIR_Assert(0);
    }
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

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
        /* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS))
    {
        /* send to intranode-rank 0 on the root's node */
        if (comm_ptr->node_comm != NULL &&
            MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
        {                                                /* and is on our node (!-1) */
            if (root == comm_ptr->rank) {
                mpi_errno = MPIC_Send(buffer,count,datatype,0,
                                         MPIR_BCAST_TAG,comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            else if (0 == comm_ptr->node_comm->rank) {
                mpi_errno = MPIC_Recv(buffer,count,datatype,MPIR_Get_intranode_rank(comm_ptr, root),
                                         MPIR_BCAST_TAG,comm_ptr->node_comm, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                /* check that we received as much as we expected */
                MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
                /* recvd_size may not be accurate for packed heterogeneous data */
                if (is_homogeneous && recvd_size != nbytes) {
                    if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
                    MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
				  "**collective_size_mismatch",
				  "**collective_size_mismatch %d %d", 
				  recvd_size, nbytes );
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            
        }

        /* perform the internode broadcast */
        if (comm_ptr->node_roots_comm != NULL)
        {
            mpi_errno = MPID_Bcast(buffer, count, datatype,
                                   MPIR_Get_internode_rank(comm_ptr, root),
                                   comm_ptr->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* perform the intranode broadcast on all except for the root's node */
        if (comm_ptr->node_comm != NULL)
        {
            mpi_errno = MPID_Bcast(buffer, count, datatype, 0,
                                   comm_ptr->node_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }
    else /* (nbytes > MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_ptr->size >= MPIR_CVAR_BCAST_MIN_PROCS) */
    {
        /* supposedly...
           smp+doubling good for pof2
           reg+ring better for non-pof2 */
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL))
        {
            /* medium-sized msg and pof2 np */

            /* perform the intranode broadcast on the root's node */
            if (comm_ptr->node_comm != NULL &&
                MPIR_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */
            {                                                /* and is on our node (!-1) */
                /* FIXME binomial may not be the best algorithm for on-node
                   bcast.  We need a more comprehensive system for selecting the
                   right algorithms here. */
                mpi_errno = MPID_Bcast(buffer, count, datatype,
                                       MPIR_Get_intranode_rank(comm_ptr, root),
                                       comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            /* perform the internode broadcast */
            if (comm_ptr->node_roots_comm != NULL)
            {
                mpi_errno = MPID_Bcast(buffer, count, datatype,
                                       MPIR_Get_internode_rank(comm_ptr, root),
                                       comm_ptr->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            /* perform the intranode broadcast on all except for the root's node */
            if (comm_ptr->node_comm != NULL &&
                MPIR_Get_intranode_rank(comm_ptr, root) <= 0) /* 0 if root was local root too */
            {                                                 /* -1 if different node than root */
                /* FIXME binomial may not be the best algorithm for on-node
                   bcast.  We need a more comprehensive system for selecting the
                   right algorithms here. */
                mpi_errno = MPID_Bcast(buffer, count, datatype, 0,
                                       comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
        else /* large msg or non-pof2 */
        {
            /* FIXME It would be good to have an SMP-aware version of this
               algorithm that (at least approximately) minimized internode
               communication. */
            mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

fn_exit:
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;

fn_fail:
    goto fn_exit;
}


/* This is the default implementation of broadcast. The algorithm is:
   
   Algorithm: MPI_Bcast

   For short messages, we use a binomial tree algorithm. 
   Cost = lgp.alpha + n.lgp.beta

   For long messages, we do a scatter followed by an allgather. 
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

   Note that this algorithm has twice the latency as the tree algorithm
   we use for short messages, but requires lower bandwidth: 2.n.beta
   versus n.lgp.beta. Therefore, for long messages and when lgp > 2,
   this algorithm will perform better.

   For long messages and for medium-size messages and non-power-of-two 
   processes, we use a ring algorithm for the allgather, which 
   takes p-1 steps, because it performs better than recursive doubling.
   Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta

   Possible improvements: 
   For clusters of SMPs, we may want to do something differently to
   take advantage of shared memory on each node.

   End Algorithm: MPI_Bcast
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* not declared static because it is called in intercomm. allgatherv */
int MPIR_Bcast_intra ( 
        void *buffer, 
        int count, 
        MPI_Datatype datatype, 
        int root, 
        MPIR_Comm *comm_ptr,
        MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size;
    int nbytes=0;
    int is_homogeneous;
    MPI_Aint type_size;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_BCAST);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_BCAST);

    if (count == 0) goto fn_exit;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE ? type_size*count : 0;
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BCAST &&
        nbytes <= MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE && MPIR_Comm_is_node_aware(comm_ptr)) {
        mpi_errno = smp_bcast(buffer, count, datatype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

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
	/* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS))
    {
        mpi_errno = MPIR_Bcast_binomial(buffer, count, datatype, root, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */
    {
        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPIU_is_pof2(comm_size, NULL)))
        {
            mpi_errno = MPIR_Bcast_scatter_doubling_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        else /* (nbytes >= MPIR_CVAR_BCAST_LONG_MSG_SIZE) || !(comm_size_is_pof2) */
        {
            /* We want the ring algorithm whether or not we have a
               topologically aware communicator.  Doing inter/intra-node
               communication phases breaks the pipelining of the algorithm.  */
            mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_BCAST);

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* Not PMPI_LOCAL because it is called in intercomm allgather */
int MPIR_Bcast_inter ( 
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
/*  Intercommunicator broadcast.
    Root sends to rank 0 in remote group. Remote group does local
    intracommunicator broadcast.
*/
    int rank, mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Status status;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_BCAST_INTER);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_BCAST_INTER);


    if (root == MPI_PROC_NULL)
    {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    }
    else if (root == MPI_ROOT)
    {
        /* root sends to rank 0 on remote group and returns */
        mpi_errno =  MPIC_Send(buffer, count, datatype, 0,
                                  MPIR_BCAST_TAG, comm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else
    {
        /* remote group. rank 0 on remote group receives from root */
        
        rank = comm_ptr->rank;
        
        if (rank == 0)
        {
            mpi_errno = MPIC_Recv(buffer, count, datatype, root,
                                     MPIR_BCAST_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPII_Setup_intercomm_localcomm( comm_ptr );

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual broadcast on this intracommunicator
           with rank 0 as root. */
        mpi_errno = MPIR_Bcast_intra(buffer, count, datatype, 0, newcomm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_BCAST_INTER);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}

/* MPIR_Bcast performs an broadcast using point-to-point messages.
   This is intended to be used by device-specific implementations of
   broadcast. */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        int use_coll = MPIR_CVAR_USE_BCAST;
        switch (use_coll) {
            case MPIR_BCAST_INTRA:
                mpi_errno = MPIR_Bcast_intra( buffer, count, datatype, root, comm_ptr, errflag );
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
            case MPIR_MPICH_TREE_BCAST_KNOMIAL:
                mpi_errno = MPIC_MPICH_TREE_bcast (buffer, count, datatype, root, 
                        &(MPIC_COMM(comm_ptr)->mpich_tree), (int*) errflag, 0, MPIR_CVAR_BCAST_TREE_KVAL, -1);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
            case MPIR_MPICH_TREE_BCAST_KARY:
                mpi_errno = MPIC_MPICH_TREE_bcast (buffer, count, datatype, root, 
                        &(MPIC_COMM(comm_ptr)->mpich_tree), (int*) errflag, 1, MPIR_CVAR_BCAST_TREE_KVAL, -1);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
            default:
                mpi_errno = MPIR_Bcast_intra( buffer, count, datatype, root, comm_ptr, errflag );
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
        }
        
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Bcast_inter( buffer, count, datatype, root, comm_ptr, errflag );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@
MPI_Bcast - Broadcasts a message from the process with rank "root" to
            all other processes of the communicator

Input/Output Parameters:
. buffer - starting address of buffer (choice) 

Input Parameters:
+ count - number of entries in buffer (integer) 
. datatype - data type of buffer (handle) 
. root - rank of broadcast root (integer) 
- comm - communicator (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_ROOT
@*/
int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, 
               MPI_Comm comm )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_BCAST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_BCAST);

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
	    
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
	    }
	    else {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
	    }
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr( datatype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_BUF_INPLACE(buffer, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(buffer,count,datatype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Bcast( buffer, count, datatype, root, comm_ptr, &errflag );
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_BCAST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_bcast",
	    "**mpi_bcast %p %d %D %d %C", buffer, count, datatype, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
