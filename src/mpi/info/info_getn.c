/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpiinfo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_get_nkeys */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_get_nkeys = PMPI_Info_get_nkeys
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_get_nkeys  MPI_Info_get_nkeys
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_get_nkeys as PMPI_Info_get_nkeys
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys) __attribute__((weak,alias("PMPI_Info_get_nkeys")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_get_nkeys
#define MPI_Info_get_nkeys PMPI_Info_get_nkeys

#undef FUNCNAME
#define FUNCNAME MPIR_Info_get_nkeys_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void MPIR_Info_get_nkeys_impl(MPID_Info *info_ptr, int *nkeys)
{
    int n;
    
    info_ptr = info_ptr->next;
    n = 0;

    while (info_ptr) {
        info_ptr = info_ptr->next;
        n ++;
    }
    *nkeys = n;

    return;
}

#endif


/*@
    MPI_Info_get_nkeys - Returns the number of currently defined keys in info

Input Parameters:
. info - info object (handle)

Output Parameters:
. nkeys - number of defined keys (integer)

.N ThreadSafeInfoRead

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_OTHER
@*/
#undef FUNCNAME
#define FUNCNAME MPI_Info_get_nkeys
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPI_Info_get_nkeys( MPI_Info info, int *nkeys )
{
    MPID_Info *info_ptr=0;
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INFO_GET_NKEYS);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INFO_GET_NKEYS);
    
    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INFO(info, mpi_errno);
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
	    
            MPIR_ERRTEST_ARGNULL(nkeys,"nkeys",mpi_errno);

        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    MPIR_Info_get_nkeys_impl(info_ptr, nkeys);
    
    /* ... end of body of routine ... */

#ifdef HAVE_ERROR_CHECKING
  fn_exit:
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INFO_GET_NKEYS);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
    
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
  fn_fail:
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_info_get_nkeys",
	    "**mpi_info_get_nkeys %I %p", info, nkeys);
    }
    mpi_errno = MPIR_Err_return_comm( NULL, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
#   endif
}
