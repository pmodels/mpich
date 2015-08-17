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
    - name        : MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS
      category    : COLLECTIVE
      type        : int
      default     : 32
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Use Ssend (synchronous send) for intercommunicator MPI_Gatherv if the
        "group B" size is >= this value.  Specifying "-1" always avoids using
        Ssend.  For backwards compatibility, specifying "0" uses the default
        value.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Gatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Gatherv = PMPI_Gatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Gatherv  MPI_Gatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Gatherv as PMPI_Gatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root,
                MPI_Comm comm)
                __attribute__((weak,alias("PMPI_Gatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Gatherv
#define MPI_Gatherv PMPI_Gatherv

/* This is the default implementation of gatherv. The algorithm is:
   
   Algorithm: MPI_Gatherv

   Since the array of recvcounts is valid only on the root, we cannot
   do a tree algorithm without first communicating the recvcounts to
   other processes. Therefore, we simply use a linear algorithm for the
   gather, which takes (p-1) steps versus lgp steps for the tree
   algorithm. The bandwidth requirement is the same for both algorithms.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements: 

   End Algorithm: MPI_Gatherv
*/

/* not declared static because it is called in intercommunicator allgatherv */
/* MPIR_Gatherv performs an gatherv using point-to-point messages.
   This is intended to be used by device-specific implementations of
   gatherv.  In all other cases MPIR_Gatherv_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gatherv ( 
	const void *sendbuf,
	int sendcount,
	MPI_Datatype sendtype,
	void *recvbuf,
	const int *recvcounts,
	const int *displs,
	MPI_Datatype recvtype,
	int root,
	MPID_Comm *comm_ptr,
        MPIR_Errflag_t *errflag )
{
    int        comm_size, rank;
    int        mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint       extent;
    int            i, reqs;
    int min_procs;
    MPID_Request **reqarray;
    MPI_Status *starray;
    MPIU_CHKLMEM_DECL(2);

    rank = comm_ptr->rank;
    
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPID_INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPID_INTERCOMM) && (root == MPI_ROOT))) {
        if (comm_ptr->comm_kind == MPID_INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPID_Datatype_get_extent_macro(recvtype, extent);
	/* each node can make sure it is not going to overflow aint */
        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
					 displs[rank] * extent);

        MPIU_CHKLMEM_MALLOC(reqarray, MPID_Request **, comm_size * sizeof(MPID_Request *), mpi_errno, "reqarray");
        MPIU_CHKLMEM_MALLOC(starray, MPI_Status *, comm_size * sizeof(MPI_Status), mpi_errno, "starray");

        reqs = 0;
        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPID_INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                                   ((char *)recvbuf+displs[rank]*extent), 
                                                   recvcounts[rank], recvtype);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
                else {
                    mpi_errno = MPIC_Irecv(((char *)recvbuf+displs[i]*extent),
                                              recvcounts[i], recvtype, i,
                                              MPIR_GATHERV_TAG, comm_ptr,
                                              &reqarray[reqs++]);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        /* ... then wait for *all* of them to finish: */
        mpi_errno = MPIC_Waitall(reqs, reqarray, starray, errflag);
        if (mpi_errno&& mpi_errno != MPI_ERR_IN_STATUS) MPIR_ERR_POP(mpi_errno);
        
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno == MPI_ERR_IN_STATUS) {
            for (i = 0; i < reqs; i++) {
                if (starray[i].MPI_ERROR != MPI_SUCCESS) {
                    mpi_errno = starray[i].MPI_ERROR;
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            }
        }
        /* --END ERROR HANDLING-- */
    }

    else if (root != MPI_PROC_NULL) { /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (sendcount) {
            /* we want local size in both the intracomm and intercomm cases
               because the size of the root's group (group A in the standard) is
               irrelevant here. */
            comm_size = comm_ptr->local_size;

            min_procs = MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS;
            if (min_procs == -1)
                min_procs = comm_size + 1; /* Disable ssend */
            else if (min_procs == 0) /* backwards compatibility, use default value */
                MPIR_CVAR_GET_DEFAULT_INT(MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS,&min_procs);

            if (comm_size >= min_procs) {
                mpi_errno = MPIC_Ssend(sendbuf, sendcount, sendtype, root,
                                          MPIR_GATHERV_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            else {
                mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, root,
                                         MPIR_GATHERV_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
    }
    

fn_exit:
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    MPIU_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* MPIR_Gatherv_impl should be called by any internal component that
   would otherwise call MPI_Gatherv.  This differs from MPIR_Gatherv
   in that this will call the coll_fns version if it exists.  This
   function replaces NMPI_Gatherv. */
#undef FUNCNAME
#define FUNCNAME MPIR_Gatherv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                      void *recvbuf, const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                      int root, MPID_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Gatherv != NULL) {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Gatherv(sendbuf, sendcount, sendtype,
                                                recvbuf, recvcounts, displs, recvtype,
                                                root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	/* --END USEREXTENSION-- */
    } else {
        mpi_errno = MPIR_Gatherv(sendbuf, sendcount, sendtype,
                                 recvbuf, recvcounts, displs, recvtype,
                                 root, comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Gatherv - Gathers into specified locations from all processes in a group

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. sendcount - number of elements in send buffer (integer) 
. sendtype - data type of send buffer elements (handle) 
. recvcounts - integer array (of length group size) 
containing the number of elements that are received from each process
(significant only at 'root') 
. displs - integer array (of length group size). Entry 
 'i'  specifies the displacement relative to recvbuf  at
which to place the incoming data from process  'i'  (significant only
at root) 
. recvtype - data type of recv buffer elements 
(significant only at 'root') (handle) 
. root - rank of receiving process (integer) 
- comm - communicator (handle) 

Output Parameters:
. recvbuf - address of receive buffer (choice, significant only at 'root') 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, const int *recvcounts, const int *displs,
                MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GATHERV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_GATHERV);

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
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
            int i, rank, comm_size;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                if (sendbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }

                rank = comm_ptr->rank;
                if (rank == root) {
                    comm_size = comm_ptr->local_size;
                    for (i=0; i<comm_size; i++) {
                        MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }

                    for (i=0; i<comm_size; i++) {
                        if (recvcounts[i] > 0) {
                            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[i],recvtype,mpi_errno);
                            break;
                        }
                    }

                    /* catch common aliasing cases */
                    if (sendbuf != MPI_IN_PLACE && sendtype == recvtype && recvcounts[comm_ptr->rank] != 0 && sendcount != 0) {
                        int recvtype_size;
                        MPID_Datatype_get_size_macro(recvtype, recvtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + displs[comm_ptr->rank]*recvtype_size, mpi_errno);
                    }
                }
                else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }

	    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    comm_size = comm_ptr->remote_size;
                    for (i=0; i<comm_size; i++) {
                        MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    for (i=0; i<comm_size; i++) {
                        if (recvcounts[i] > 0) {
                            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[i],recvtype,mpi_errno);
                            break;
                        }
                    }
                }
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype,
                                  recvbuf, recvcounts, displs, recvtype,
                                  root, comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_GATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_gatherv",
	    "**mpi_gatherv %p %d %D %p %p %p %D %d %C", sendbuf, sendcount, sendtype,
	    recvbuf, recvcounts, displs, recvtype, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
