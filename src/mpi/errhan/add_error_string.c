/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Add_error_string */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Add_error_string = PMPI_Add_error_string
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Add_error_string  MPI_Add_error_string
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Add_error_string as PMPI_Add_error_string
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Add_error_string(int errorcode, const char *string) __attribute__((weak,alias("PMPI_Add_error_string")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Add_error_string
#define MPI_Add_error_string PMPI_Add_error_string

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Add_error_string

/*@
   MPI_Add_error_string - Associates an error string with an MPI error code or 
   class

Input Parameters:
+ errorcode - error code or class (integer) 
- string - text corresponding to errorcode (string) 

   Notes:
The string must be no more than 'MPI_MAX_ERROR_STRING' characters long. 
The length of the string is as defined in the calling language. 
The length of the string does not include the null terminator in C or C++.  
Note that the string is 'const' even though the MPI standard does not 
specify it that way.

According to the MPI-2 standard, it is erroneous to call 'MPI_Add_error_string'
for an error code or class with a value less than or equal 
to 'MPI_ERR_LASTCODE'.  Thus, you cannot replace the predefined error messages
with this routine.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Add_error_string(int errorcode, const char *string)
{
    static const char FCNAME[] = "MPI_Add_error_string";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ADD_ERROR_STRING);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ADD_ERROR_STRING);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(string,"string",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPIR_Err_set_msg( errorcode, (const char *)string );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_STRING);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_add_error_string",
	    "**mpi_add_error_string %d %s", errorcode, string);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

