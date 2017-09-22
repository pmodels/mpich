/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_size = PMPI_Group_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_size  MPI_Group_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_size as PMPI_Group_size
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Group_size(MPI_Group group, int *size) __attribute__((weak,alias("PMPI_Group_size")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_size
#define MPI_Group_size PMPI_Group_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_size
#undef FCNAME
#define FCNAME "MPI_Group_size"

/*@

MPI_Group_size - Returns the size of a group

Input Parameters:
+ group - group (handle) 

Output Parameters:
- size - number of processes in the group (integer) 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_GROUP
.N MPI_ERR_ARG
@*/
int MPI_Group_size(MPI_Group group, int *size)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Group *group_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GROUP_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GROUP_SIZE);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_GROUP(group, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPIR_Group_get_ptr( group, group_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPIR_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not value, it will be reset to null */
            if (mpi_errno) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(size, "size", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    *size = group_ptr->size;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GROUP_SIZE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_group_size",
	    "**mpi_group_size %G %p", group, size);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

