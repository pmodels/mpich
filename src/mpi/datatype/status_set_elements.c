/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Status_set_elements */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Status_set_elements = PMPI_Status_set_elements
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Status_set_elements  MPI_Status_set_elements
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Status_set_elements as PMPI_Status_set_elements
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count) __attribute__((weak,alias("PMPI_Status_set_elements")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Status_set_elements
#define MPI_Status_set_elements PMPI_Status_set_elements

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Status_set_elements
#undef FCNAME
#define FCNAME "MPI_Status_set_elements"

/*@
   MPI_Status_set_elements - Set the number of elements in a status

Input Parameters:
+ status - status to associate count with (Status) 
. datatype - datatype associated with count (handle) 
- count - number of elements to associate with status (integer) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_TYPE
@*/
int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, 
			    int count)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_STATUS_SET_ELEMENTS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_STATUS_SET_ELEMENTS);
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(count,mpi_errno);
	    MPIR_ERRTEST_ARGNULL(status,"status",mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* Validate datatype_ptr */
	    MPIR_Datatype_get_ptr( datatype, datatype_ptr );
            MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
	    /* If datatype_ptr is not valid, it will be reset to null */
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPIR_Status_set_elements_x_impl(status, datatype, (MPI_Count)count);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_STATUS_SET_ELEMENTS);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_status_set_elements",
	    "**mpi_status_set_elements %p %D %d", status, datatype, count);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
