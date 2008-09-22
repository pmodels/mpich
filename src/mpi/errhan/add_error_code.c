/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "errcodes.h"

/* -- Begin Profiling Symbol Block for routine MPI_Add_error_code */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Add_error_code = PMPI_Add_error_code
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Add_error_code  MPI_Add_error_code
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Add_error_code as PMPI_Add_error_code
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Add_error_code
#define MPI_Add_error_code PMPI_Add_error_code

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Add_error_code

/*@
   MPI_Add_error_code - Add and MPI error code to an MPI error class

   Input Parameter:
.  errorclass - Error class to add an error code.

   Output Parameter:
.  errorcode - New error code for this error class.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Add_error_code(int errorclass, int *errorcode)
{
    static const char FCNAME[] = "MPI_Add_error_code";
    int mpi_errno = MPI_SUCCESS;
    int new_code;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ADD_ERROR_CODE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ADD_ERROR_CODE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* FIXME: verify that errorclass is a dynamic class */
	    MPIR_ERRTEST_ARGNULL(errorcode, "errorcode", mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */
    
    new_code = MPIR_Err_add_code( errorclass );
    MPIU_ERR_CHKANDJUMP(new_code<0,mpi_errno,MPI_ERR_OTHER,"**noerrcodes");

    *errorcode = new_code;
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ADD_ERROR_CODE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_add_error_code",
	    "**mpi_add_error_code %d %p", errorclass, errorcode);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

