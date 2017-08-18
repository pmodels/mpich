/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_impl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_USE_BARRIER
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Controls barrier algorithm:
        0 - Default barrier (old mpir barrier)
        2 - Knomial tree based barrier with non-blocking transport
        3 - Kary tree based barrier with non-blocking transport
        4 - Knomial tree based barrier with blocking transport
        5 - Kary tree based barrier with blocking transport

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

enum {
    BARRIER_DEFAULT = 0,
    BARRIER_TREE_KNOMIAL_NONBLOCKING_TRANSPORT,
    BARRIER_TREE_KARY_NONBLOCING_TRANSPORT,
    BARRIER_TREE_KNOMIAL_BLOCKING_TRANSPORT,
    BARRIER_TREE_KARY_BLOCKING_TRANSPORT
};

/* -- Begin Profiling Symbol Block for routine MPI_Barrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Barrier = PMPI_Barrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Barrier  MPI_Barrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Barrier as PMPI_Barrier
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Barrier(MPI_Comm comm) __attribute__((weak,alias("PMPI_Barrier")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Barrier
#define MPI_Barrier PMPI_Barrier

#undef FUNCNAME
#define FUNCNAME MPIR_Barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Barrier(MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    int use_coll = MPIR_CVAR_USE_BARRIER;
    if(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
        use_coll = 0;

    switch (use_coll) {
        case BARRIER_DEFAULT:
            mpi_errno = MPIC_DEFAULT_Barrier( comm_ptr, errflag );
            break;
        case BARRIER_TREE_KNOMIAL_NONBLOCKING_TRANSPORT:
            mpi_errno = MPIC_MPICH_TREE_barrier(&(MPIC_COMM(comm_ptr)->mpich_tree), (int *) errflag, 0, 2);
            break;
        case BARRIER_TREE_KARY_NONBLOCING_TRANSPORT:
            mpi_errno = MPIC_MPICH_TREE_barrier(&(MPIC_COMM(comm_ptr)->mpich_tree), (int *) errflag, 1, 2);
            break;
        default:
            mpi_errno = MPIC_DEFAULT_Barrier( comm_ptr, errflag );
            break;
    }
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif




#undef FUNCNAME
#define FUNCNAME MPI_Barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/*@

MPI_Barrier - Blocks until all processes in the communicator have
reached this routine.  

Input Parameters:
. comm - communicator (handle) 

Notes:
Blocks the caller until all processes in the communicator have called it; 
that is, the call returns at any process only after all members of the
communicator have entered the call.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Barrier( MPI_Comm comm )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_BARRIER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_BARRIER);
    
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
	    /* Validate communicator */
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Barrier(comm_ptr, &errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_BARRIER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_barrier", "**mpi_barrier %C", comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
