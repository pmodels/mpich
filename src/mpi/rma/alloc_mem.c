/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Alloc_mem */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alloc_mem = PMPI_Alloc_mem
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alloc_mem  MPI_Alloc_mem
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alloc_mem as PMPI_Alloc_mem
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr) __attribute__((weak,alias("PMPI_Alloc_mem")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Alloc_mem
#define MPI_Alloc_mem PMPI_Alloc_mem

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alloc_mem

/*@
   MPI_Alloc_mem - Allocate memory for message passing and RMA

Input Parameters:
+ size - size of memory segment in bytes (nonnegative integer) 
- info - info argument (handle) 

Output Parameters:
. baseptr - pointer to beginning of memory segment allocated 

   Notes:
 Using this routine from Fortran requires that the Fortran compiler accept
 a common pointer extension.  See Section 4.11 (Memory Allocation) in the
 MPI-2 standard for more information and examples.

   Also note that while 'baseptr' is a 'void *' type, this is 
   simply to allow easy use of any pointer object for this parameter.  
   In fact, this argument is really a 'void **' type, that is, a 
   pointer to a pointer. 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INFO
.N MPI_ERR_ARG
.N MPI_ERR_NO_MEM
@*/
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
    static const char FCNAME[] = "MPI_Alloc_mem";
    int mpi_errno = MPI_SUCCESS;
    void *ap;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLOC_MEM);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ALLOC_MEM);
    
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_ARGNEG(size, "size", mpi_errno);
	    MPIR_ERRTEST_ARGNULL(baseptr, "baseptr", mpi_errno);
            MPIR_ERRTEST_INFO_OR_NULL(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Info_get_ptr( info, info_ptr );

    /* ... body of routine ...  */

    MPIU_Ensure_Aint_fits_in_pointer(size);
    ap = MPID_Alloc_mem(size, info_ptr);

    /* --BEGIN ERROR HANDLING-- */
    if (!ap)
    {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_NO_MEM, "**allocmem", 0 );
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    *(void **)baseptr = ap;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLOC_MEM);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_alloc_mem",
	    "**mpi_alloc_mem %d %I %p", size, info, baseptr);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
