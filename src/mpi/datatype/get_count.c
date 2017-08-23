/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_count */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_count = PMPI_Get_count
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_count  MPI_Get_count
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_count as PMPI_Get_count
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count) __attribute__((weak,alias("PMPI_Get_count")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_count
#define MPI_Get_count PMPI_Get_count

#undef FUNCNAME
#define FUNCNAME MPIR_Get_count_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Get_count_impl(const MPI_Status *status, MPI_Datatype datatype, int *count)
{
    MPI_Count size;

    MPIR_Datatype_get_size_macro(datatype, size);
    MPIR_Assert(size >= 0 && MPIR_STATUS_GET_COUNT(*status) >= 0);
    if (size != 0) {
        /* MPI-3 says return MPI_UNDEFINED if too large for an int */
	if ((MPIR_STATUS_GET_COUNT(*status) % size) != 0 || ((MPIR_STATUS_GET_COUNT(*status) / size) > INT_MAX))
	    (*count) = MPI_UNDEFINED;
	else
	    (*count) = (int)(MPIR_STATUS_GET_COUNT(*status) / size);
    } else {
	if (MPIR_STATUS_GET_COUNT(*status) > 0) {
	    /* --BEGIN ERROR HANDLING-- */

	    /* case where datatype size is 0 and count is > 0 should
	     * never occur.
	     */

	    (*count) = MPI_UNDEFINED;
	    /* --END ERROR HANDLING-- */
	} else {
	    /* This is ambiguous.  However, discussions on MPI Forum
	       reached a consensus that this is the correct return 
	       value
	    */
	    (*count) = 0;
	}
    }
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_count
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
  MPI_Get_count - Gets the number of "top level" elements

Input Parameters:
+ status - return status of receive operation (Status) 
- datatype - datatype of each receive buffer element (handle) 

Output Parameters:
. count - number of received elements (integer) 
Notes:
If the size of the datatype is zero, this routine will return a count of
zero.  If the amount of data in 'status' is not an exact multiple of the 
size of 'datatype' (so that 'count' would not be integral), a 'count' of
'MPI_UNDEFINED' is returned instead.

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
@*/
int MPI_Get_count( const MPI_Status *status, MPI_Datatype datatype, int *count )
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_GET_COUNT);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_GET_COUNT);

#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(count, "count", mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* Validate datatype_ptr */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
		MPIR_Datatype_get_ptr(datatype, datatype_ptr);
		MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno) goto fn_fail;
		/* Q: Must the type be committed to be used with this function? */
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Get_count_impl(status, datatype, count);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_GET_COUNT);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_get_count",
	    "**mpi_get_count %p %D %p", status, datatype, count);
    }
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
