/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include <limits.h>

/* -- Begin Profiling Symbol Block for routine MPI_Type_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Type_size = PMPI_Type_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Type_size  MPI_Type_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Type_size as PMPI_Type_size
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Type_size(MPI_Datatype datatype, int *size) __attribute__((weak,alias("PMPI_Type_size")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Type_size
#define MPI_Type_size PMPI_Type_size

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Type_size
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Type_size - Return the number of bytes occupied by entries
                    in the datatype

Input Parameters:
. datatype - datatype (handle) 

Output Parameters:
. size - datatype size (integer) 

.N SignalSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Type_size(MPI_Datatype datatype, int *size)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Count size_x = MPI_UNDEFINED;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_TYPE_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_TYPE_SIZE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    /* If this is a built-in datatype, then get the size out of the handle */
    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
    {
	MPIR_Datatype_get_size_macro(datatype, *size);
	goto fn_exit;
    }

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *datatype_ptr = NULL;

            /* Convert MPI object handles to object pointers */
            MPIR_Datatype_get_ptr( datatype, datatype_ptr );

            /* Validate datatype_ptr */
            MPIR_Datatype_valid_ptr( datatype_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Type_size_x_impl(datatype, &size_x);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(size_x >= 0);
    /* handle overflow: see MPI-3 p.104 */
    *size = (size_x > INT_MAX) ? MPI_UNDEFINED : (int)size_x;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_TYPE_SIZE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_type_size", 
	    "**mpi_type_size %D %p", datatype, size);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
