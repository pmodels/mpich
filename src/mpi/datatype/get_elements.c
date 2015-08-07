/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_elements */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_elements = PMPI_Get_elements
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_elements  MPI_Get_elements
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_elements as PMPI_Get_elements
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count) __attribute__((weak,alias("PMPI_Get_elements")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_elements
#define MPI_Get_elements PMPI_Get_elements

/* any non-MPI functions go here, especially non-static ones */

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_elements
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Get_elements - Returns the number of basic elements
                      in a datatype

Input Parameters:
+ status - return status of receive operation (Status)
- datatype - datatype used by receive operation (handle)

Output Parameters:
. count - number of received basic elements (integer)

   Notes:

 If the size of the datatype is zero and the amount of data returned as
 determined by 'status' is also zero, this routine will return a count of
 zero.  This is consistent with a clarification made by the MPI Forum.

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Count count_x;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_ELEMENTS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ELEMENTS);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(count, "count", mpi_errno);
            /* Convert MPI object handles to object pointers */
            MPID_Datatype_get_ptr(datatype, datatype_ptr);
            /* Validate datatype_ptr */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;

                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif

    /* ... body of routine ...  */

    mpi_errno = MPIR_Get_elements_x_impl(status, datatype, &count_x);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* clip the value if it cannot be correctly returned to the user */
    *count = (count_x > INT_MAX) ? MPI_UNDEFINED : (int)count_x;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ELEMENTS);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_get_elements",
	    "**mpi_get_elements %p %D %p", status, datatype, count);
    }
    mpi_errno = MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
