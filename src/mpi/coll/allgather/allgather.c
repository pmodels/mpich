/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 81920
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the short message algorithm will
        be used if the send buffer size is < this value (in bytes).
        (See also: MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the long message algorithm will be
        used if the send buffer size is >= this value (in bytes)
        (See also: MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Allgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allgather = PMPI_Allgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allgather  MPI_Allgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allgather as PMPI_Allgather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                  __attribute__((weak,alias("PMPI_Allgather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allgather
#define MPI_Allgather PMPI_Allgather

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

/* not declared static because a machine-specific function may call this 
   one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_intra ( 
    const void *sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcount,
    MPI_Datatype recvtype,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint   recvtype_extent, tot_bytes;
    int type_size;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_extent_macro( recvtype, recvtype_extent );
    MPIR_Datatype_get_size_macro( recvtype, type_size );

    /* This is the largest offset we add to recvbuf */
    MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
				     (comm_size * recvcount * recvtype_extent));

    tot_bytes = (MPI_Aint)recvcount * comm_size * type_size;
    if ((tot_bytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        /* Short or medium size message and power-of-two no. of processes. Use
         * recursive doubling algorithm */
        MPIR_Allgather_recursive_doubling (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    else if (tot_bytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        /* Short message and non-power-of-two no. of processes. Use
         * Bruck algorithm (see description above). */
        MPIR_Allgather_brucks (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    else {  /* long message or medium-size message and non-power-of-two
             * no. of processes. use ring algorithm. */
        MPIR_Allgather_ring (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, errflag); 
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* not declared static because a machine-specific function may call this one 
   in some cases */
int MPIR_Allgather_inter ( 
    const void *sendbuf,
    int sendcount,
    MPI_Datatype sendtype,
    void *recvbuf,
    int recvcount,
    MPI_Datatype recvtype,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
    /* Intercommunicator Allgather.
       Each group does a gather to local root with the local
       intracommunicator, and then does an intercommunicator broadcast.
    */

    int rank, local_size, remote_size, mpi_errno = MPI_SUCCESS, root;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb = 0, extent, send_extent;
    void *tmp_buf=NULL;
    MPIR_Comm *newcomm_ptr = NULL;

    MPIR_CHKLMEM_DECL(1);

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
        MPIR_CHKLMEM_MALLOC(tmp_buf, void*, extent*sendcount*local_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
	MPII_Setup_intercomm_localcomm( comm_ptr );

    newcomm_ptr = comm_ptr->local_comm;

    if (sendcount != 0) {
        mpi_errno = MPID_Gather(sendbuf, sendcount, sendtype, tmp_buf, sendcount,
                                     sendtype, 0, newcomm_ptr, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* first broadcast from left to right group, then from right to
       left group */
    if (comm_ptr->is_low_group) {
        /* bcast to right*/
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPID_Bcast(tmp_buf, sendcount*local_size,
                                   sendtype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* receive bcast from right */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPID_Bcast(recvbuf, recvcount*remote_size,
                                   recvtype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }
    else {
        /* receive bcast from left */
        if (recvcount != 0) {
            root = 0;
            mpi_errno = MPID_Bcast(recvbuf, recvcount*remote_size,
                                   recvtype, root, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        /* bcast to left */
        if (sendcount != 0) {
            root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
            mpi_errno = MPID_Bcast(tmp_buf, sendcount*local_size,
                                   sendtype, root, comm_ptr, errflag);
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


/* MPIR_Allgather performs an allgather using point-to-point messages.
   This is intended to be used by device-specific implementations of
   allgather. */
#undef FUNCNAME
#define FUNCNAME MPIR_Allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Allgather_intra(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype,
                                         comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno = MPIR_Allgather_inter(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype,
                                         comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif


#undef FUNCNAME
#define FUNCNAME MPI_Allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Allgather - Gathers data from all tasks and distribute the combined
    data to all tasks

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcount - number of elements in send buffer (integer) 
. sendtype - data type of send buffer elements (handle) 
. recvcount - number of elements received from any process (integer) 
. recvtype - data type of receive buffer elements (handle) 
- comm - communicator (handle) 

Output Parameters:
. recvbuf - address of receive buffer (choice) 

Notes:
 The MPI standard (1.0 and 1.1) says that 
.n
.n
 The jth block of data sent from  each process is received by every process 
 and placed in the jth block of the buffer 'recvbuf'.  
.n
.n
 This is misleading; a better description is
.n
.n
 The block of data sent from the jth process is received by every
 process and placed in the jth block of the buffer 'recvbuf'.
.n
.n
 This text was suggested by Rajeev Thakur and has been adopted as a 
 clarification by the MPI Forum.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLGATHER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLGATHER);

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
            MPIR_Datatype *recvtype_ptr=NULL, *sendtype_ptr=NULL;

            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            } else {
                /* catch common aliasing cases */
                if (sendbuf != MPI_IN_PLACE && sendtype == recvtype &&
                        recvcount != 0 && sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + comm_ptr->rank*recvcount*recvtype_size, mpi_errno);
                }
            }

            if (sendbuf != MPI_IN_PLACE)
	    {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN)
		{
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    MPIR_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                }
                MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
	    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN)
	    {
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }
	    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Allgather(sendbuf, sendcount, sendtype,
                               recvbuf, recvcount, recvtype,
                               comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;
    
    /* ... end of body of routine ... */
    
  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_allgather",
	    "**mpi_allgather %p %d %D %p %d %D %C", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
