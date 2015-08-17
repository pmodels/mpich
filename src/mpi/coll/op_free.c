/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Op_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Op_free = PMPI_Op_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Op_free  MPI_Op_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Op_free as PMPI_Op_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Op_free(MPI_Op *op) __attribute__((weak,alias("PMPI_Op_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Op_free
#define MPI_Op_free PMPI_Op_free

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Op_free

/*@
  MPI_Op_free - Frees a user-defined combination function handle
 
Input Parameters:
. op - operation (handle) 

  Notes:
  'op' is set to 'MPI_OP_NULL' on exit.

.N NULL

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_PERM_OP

.seealso: MPI_Op_create
@*/
int MPI_Op_free(MPI_Op *op)
{
#ifdef HAVE_ERROR_CHECKING
    static const char FCNAME[] = "MPI_Op_free";
#endif
    MPID_Op *op_ptr = NULL;
    int     in_use;
    int     mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_OP_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_OP_FREE);
    
    MPID_Op_get_ptr( *op, op_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPID_Op_valid_ptr( op_ptr, mpi_errno );
	    if (!mpi_errno) {
		if (op_ptr->kind < MPID_OP_USER_NONCOMMUTE) {
		    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                         MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP,
						      "**permop", 0 );
		}
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */
    
    MPIR_Op_release_ref( op_ptr, &in_use);
    if (!in_use) {
	MPIU_Handle_obj_free( &MPID_Op_mem, op_ptr );
    }
    *op = MPI_OP_NULL;
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_OP_FREE);
        MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
	return mpi_errno;
	
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_op_free", "**mpi_op_free %p", op);
    }
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}

