/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

#include <string.h>

/* -- Begin Profiling Symbol Block for routine MPI_Info_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_dup = PMPI_Info_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_dup  MPI_Info_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_dup as PMPI_Info_dup
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_dup
#define MPI_Info_dup PMPI_Info_dup
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Info_dup

/*@
    MPI_Info_dup - Returns a duplicate of the info object

Input Parameters:
. info - info object (handle)

Output Parameters:
. newinfo - duplicate of info object (handle)

.N ThreadSafeInfoRead

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
int MPI_Info_dup( MPI_Info info, MPI_Info *newinfo )
{
    MPID_Info *info_ptr=0, *curr_old, *curr_new;
    static const char FCNAME[] = "MPI_Info_dup";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_DUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_DUP);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INFO(info, mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Info_get_ptr( info, info_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate info_ptr */
            MPID_Info_valid_ptr( info_ptr, mpi_errno );
            MPIR_ERRTEST_ARGNULL(newinfo,"newinfo",mpi_errno);
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    /* Note that this routine allocates info elements one at a time.
       In the multithreaded case, each allocation may need to acquire
       and release the allocation lock.  If that is ever a problem, we
       may want to add an "allocate n elements" routine and execute this
       it two steps: count and then allocate */
    /* FIXME : multithreaded */
    mpi_errno = MPIU_Info_alloc(&curr_new);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    *newinfo = curr_new->handle;

    curr_old = info_ptr->next;
    while (curr_old)
    {
        mpi_errno = MPIU_Info_alloc(&curr_new->next);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

	curr_new	 = curr_new->next;
	curr_new->key	 = MPIU_Strdup(curr_old->key);
	curr_new->value	 = MPIU_Strdup(curr_old->value);

	curr_old	 = curr_old->next;
    }
    
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_DUP);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_info_dup",
	    "**mpi_info_dup %I %p", info, newinfo);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
