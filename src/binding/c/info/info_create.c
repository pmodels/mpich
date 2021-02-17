/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Info_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Info_create = PMPI_Info_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Info_create  MPI_Info_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Info_create as PMPI_Info_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Info_create(MPI_Info *info)  __attribute__ ((weak, alias("PMPI_Info_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Info_create
#define MPI_Info_create PMPI_Info_create

#endif

/*@
   MPI_Info_create - Creates a new info object

Output Parameters:
. info - info object created (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS

.N MPI_ERR_ARG
@*/

int MPI_Info_create(MPI_Info *info)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_INFO_CREATE);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_INFO_CREATE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_ARGNULL(info, "info", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    MPIR_Info *info_ptr = NULL;
    mpi_errno = MPIR_Info_alloc(&info_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    *info = info_ptr->handle;
    /* (info_ptr)->cookie = MPIR_INFO_COOKIE; */
    /* this is the first structure in this linked list. it is
     * always kept empty. new (key,value) pairs are added after it. */
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_INFO_CREATE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLINE-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                     "**mpi_info_create", "**mpi_info_create %p", info);
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
