/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_processor_name */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_processor_name = PMPI_Get_processor_name
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_processor_name  MPI_Get_processor_name
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_processor_name as PMPI_Get_processor_name
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Get_processor_name(char *name, int *resultlen) __attribute__((weak,alias("PMPI_Get_processor_name")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines.  You can use USE_WEAK_SYMBOLS to see if MPICH is
   using weak symbols to implement the MPI routines. */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Get_processor_name
#define MPI_Get_processor_name PMPI_Get_processor_name

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of 
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_processor_name

/*@
  MPI_Get_processor_name - Gets the name of the processor

Output Parameters:
+ name - A unique specifier for the actual (as opposed to virtual) node. This
  must be an array of size at least 'MPI_MAX_PROCESSOR_NAME'.
- resultlen - Length (in characters) of the name 

  Notes:
  The name returned should identify a particular piece of hardware; 
  the exact format is implementation defined.  This name may or may not
  be the same as might be returned by 'gethostname', 'uname', or 'sysinfo'.

.N ThreadSafe

.N Fortran

 In Fortran, the character argument should be declared as a character string
 of 'MPI_MAX_PROCESSOR_NAME' rather than an array of dimension 
 'MPI_MAX_PROCESSOR_NAME'.  That is, 
.vb
   character*(MPI_MAX_PROCESSOR_NAME) name
.ve
 rather than
.vb
   character name(MPI_MAX_PROCESSOR_NAME)
.ve
 The two 

.N FortranString

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get_processor_name( char *name, int *resultlen )
{
    static const char FCNAME[] = "MPI_Get_processor_name";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_PROCESSOR_NAME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_PROCESSOR_NAME);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNULL(name,"name",mpi_errno);
	    MPIR_ERRTEST_ARGNULL(resultlen,"resultlen",mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Get_processor_name( name, MPI_MAX_PROCESSOR_NAME, 
					 resultlen );
    
    /* ... end of body of routine ... */

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_PROCESSOR_NAME);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_get_processor_name",
	    "**mpi_get_processor_name %p %p", name, resultlen);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
