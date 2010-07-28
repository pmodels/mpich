/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Group_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Group_free = PMPI_Group_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Group_free  MPI_Group_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Group_free as PMPI_Group_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Group_free
#define MPI_Group_free PMPI_Group_free

#undef FUNCNAME
#define FUNCNAME MPIR_Group_free_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Group_free_impl(MPID_Group *group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
        
    /* Do not free MPI_GROUP_EMPTY */
    if (group_ptr->handle != MPI_GROUP_EMPTY) {
        mpi_errno = MPIR_Group_release(group_ptr);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Group_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Group_free - Frees a group

Input Parameter:
. group - group to free (handle) 

Notes:
On output, group is set to 'MPI_GROUP_NULL'.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_ARG
.N MPI_ERR_PERM_GROUP
@*/
int MPI_Group_free(MPI_Group *group)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *group_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GROUP_FREE);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GROUP_FREE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_GROUP(*group, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif
    
    /* Convert MPI object handles to object pointers */
    MPID_Group_get_ptr( *group, group_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate group_ptr */
            MPID_Group_valid_ptr( group_ptr, mpi_errno );
	    /* If group_ptr is not valid, it will be reset to null */

	    /* Cannot free the predefined groups, but allow GROUP_EMPTY 
	       because otherwise many tests fail */
	    if ((HANDLE_GET_KIND(*group) == HANDLE_KIND_BUILTIN) &&
                *group != MPI_GROUP_EMPTY) {
		mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, 
                      MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_GROUP,
					  "**groupperm", 0);
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Group_free_impl(group_ptr);
    *group = MPI_GROUP_NULL;
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GROUP_FREE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_group_free", "**mpi_group_free %p", group);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
