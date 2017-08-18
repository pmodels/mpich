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
    - name        : MPIR_CVAR_USE_BCAST
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls bcast algorithm:
        0 - Default bcast (old mpir bcast)
        1 - Knomial tree based boradcast with non-blocking transport
        2 - Kary tree based broadcast with non-blocking transport

    - name        : MPIR_CVAR_BCAST_KARY_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for Kary broadcast

    - name        : MPIR_CVAR_BCAST_KNOMIAL_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for Knomial broadcast

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/*Broadacst algorithms enumerator*/
enum {
    BCAST_DEFAULT = 0,
    BCAST_TREE_KNOMIAL_NONBLOCKING,
    BCAST_TREE_KARY_NONBLOCKING
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

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    int use_coll = MPIR_CVAR_USE_BCAST;
    if(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) /*Currently only the default can handle intercommunicators*/
        use_coll = 0;

    switch (use_coll) {
        case BCAST_DEFAULT:
            mpi_errno = MPIC_DEFAULT_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
            break;
        case BCAST_TREE_KNOMIAL_NONBLOCKING:
            mpi_errno = MPIC_MPICH_TREE_bcast(buffer, count, datatype, root,
                                              &(MPIC_COMM(comm_ptr)->mpich_tree),
                                              (int *) errflag, 0,
                                              MPIR_CVAR_BCAST_KNOMIAL_KVAL,
                                              -1);
            break;
        case BCAST_TREE_KARY_NONBLOCKING:
            mpi_errno = MPIC_MPICH_TREE_bcast(buffer, count, datatype, root,
                                              &(MPIC_COMM(comm_ptr)->mpich_tree),
                                              (int *) errflag, 1,
                                              MPIR_CVAR_BCAST_KARY_KVAL,
                                              -1);
            break;
        default:
            mpi_errno = MPIC_DEFAULT_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
            break;
    }
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

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
