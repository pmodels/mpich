/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_dup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_dup = PMPI_Info_dup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_dup  MPI_Info_dup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_dup as PMPI_Info_dup
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo)  __attribute__ ((weak, alias("PMPI_Info_dup")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_dup
#define MPI_Info_dup PMPI_Info_dup

#endif

/*@
   MPI_Info_dup - Returns a duplicate of the info object

Input Parameters:
. info - info object (handle)

Output Parameters:
. newinfo - info object (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
.N MPI_ERR_INFO
@*/

int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Info *info_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INFO_DUP);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INFO_DUP);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_INFO(info, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    MPIR_Info_get_ptr(info, info_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Info_valid_ptr(info_ptr, mpi_errno);
            if (mpi_errno) {
                goto fn_fail;
            }
            MPIR_ERRTEST_ARGNULL(newinfo, "newinfo", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Info *newinfo_ptr = NULL;
    *newinfo = MPI_INFO_NULL;
    mpi_errno = MPIR_Info_dup_impl(info_ptr, &newinfo_ptr);
    if (mpi_errno) {
        goto fn_fail;
    }
    if (newinfo_ptr) {
        MPIR_OBJ_PUBLISH_HANDLE(*newinfo, newinfo_ptr->handle);
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INFO_DUP);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_info_dup", "**mpi_info_dup %I %p", info, newinfo);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
