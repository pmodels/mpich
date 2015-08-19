/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Register_datarep */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Register_datarep = PMPI_Register_datarep
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Register_datarep  MPI_Register_datarep
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Register_datarep as PMPI_Register_datarep
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Register_datarep
#define MPI_Register_datarep PMPI_Register_datarep

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Register_datarep

/*@
   MPI_Register_datarep - Register a set of user-provided data conversion 
   functions

Input Parameters:
+ datarep - data representation identifier (string) 
. read_conversion_fn - function invoked to convert from file representation to native representation (function) 
. write_conversion_fn - function invoked to convert from native representation to file representation (function) 
. dtype_file_extent_fn - function invoked to get the extent of a datatype as represented in the file (function) 
- extra_state - extra state that is passed to the conversion functions

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
@*/
int MPI_Register_datarep(char *datarep, 
			 MPI_Datarep_conversion_function *read_conversion_fn, 
			 MPI_Datarep_conversion_function *write_conversion_fn, 
			 MPI_Datarep_extent_function *dtype_file_extent_fn, 
			 void *extra_state)
{
    static const char FCNAME[] = "MPI_Register_datarep";
    int mpi_errno = MPI_SUCCESS;
    MPID_THREADPRIV_DECL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_REGISTER_DATAREP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_REGISTER_DATAREP);
    
    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    
    /* FIXME UNIMPLEMENTED */
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);

    /* ... end of body of routine ... */
    
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_REGISTER_DATAREP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_register_datarep",
	    "**mpi_register_datarep %s %p %p %p %p", datarep, read_conversion_fn, write_conversion_fn,
	    dtype_file_extent_fn, extra_state);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
