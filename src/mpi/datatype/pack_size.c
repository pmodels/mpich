/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack_size */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack_size = PMPI_Pack_size
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack_size  MPI_Pack_size
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack_size as PMPI_Pack_size
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Pack_size
#define MPI_Pack_size PMPI_Pack_size

#undef FUNCNAME
#define FUNCNAME MPIR_Pack_size_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_Pack_size_impl(int incount, MPI_Datatype datatype, int *size)
{
    int typesize;
    MPID_Datatype_get_size_macro(datatype, typesize);
    *size = incount * typesize;
}


#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack_size
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
   MPI_Pack_size - Returns the upper bound on the amount of space needed to
                    pack a message

Input Parameters:
+ incount - count argument to packing call (integer) 
. datatype - datatype argument to packing call (handle) 
- comm - communicator argument to packing call (handle) 

Output Parameter:
. size - upper bound on size of packed message, in bytes (integer) 

Notes:
The MPI standard document describes this in terms of 'MPI_Pack', but it 
applies to both 'MPI_Pack' and 'MPI_Unpack'.  That is, the value 'size' is 
the maximum that is needed by either 'MPI_Pack' or 'MPI_Unpack'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TYPE
.N MPI_ERR_ARG

@*/
int MPI_Pack_size(int incount,
		  MPI_Datatype datatype,
		  MPI_Comm comm,
		  int *size)
{
    MPID_Comm *comm_ptr = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_PACK_SIZE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_PACK_SIZE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;

	    MPIR_ERRTEST_COUNT(incount, mpi_errno);
	    MPIR_ERRTEST_ARGNULL(size, "size", mpi_errno);
            if (mpi_errno) goto fn_fail;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno) goto fn_fail;
	    
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            if (mpi_errno) goto fn_fail;
	    
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    MPIR_Pack_size_impl(incount, datatype, size);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_PACK_SIZE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_pack_size",
	    "**mpi_pack_size %d %D %C %p", incount, datatype, comm, size);
    }
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
#   endif
    /* --END ERROR HANDLING-- */
}
