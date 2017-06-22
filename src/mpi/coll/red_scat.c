/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"
#include "coll_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_USE_REDSCAT
      category    : COLLECTIVE
      type        : int
      default     :  0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Algorithm to be used for ReduceScatter collective operation

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

enum {
    MPIC_DEFAULT_REDSCAT = 0
};

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

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Reduce_scatter_intra(const void *sendbuf, void *recvbuf, const int recvcounts[],
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Reduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);

    return mpi_errno;
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
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Reduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);

    return mpi_errno;
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
    int use_coll = MPIR_CVAR_USE_REDSCAT;
    if(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        use_coll = MPIC_DEFAULT_REDSCAT;

    switch (use_coll) {
        case MPIC_DEFAULT_REDSCAT:
        default:
            mpi_errno = MPIC_DEFAULT_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);
            break;
    }
    return mpi_errno;
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
