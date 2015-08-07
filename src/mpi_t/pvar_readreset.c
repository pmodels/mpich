/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_readreset */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_readreset = PMPI_T_pvar_readreset
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_readreset  MPI_T_pvar_readreset
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_readreset as PMPI_T_pvar_readreset
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf) __attribute__((weak,alias("PMPI_T_pvar_readreset")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_readreset
#define MPI_T_pvar_readreset PMPI_T_pvar_readreset

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_readreset_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_readreset_impl(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_T_pvar_read_impl(session, handle, buf);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    mpi_errno = MPIR_T_pvar_reset_impl(session, handle);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_readreset
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_T_pvar_readreset - Read the value of a performance variable and then reset it

Input Parameters:
+ session - identifier of performance experiment session (handle)
- handle - handle of a performance variable (handle)

Output Parameters:
. buf - initial address of storage location for variable value (choice)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_INVALID_HANDLE
.N MPI_T_ERR_PVAR_NO_WRITE
.N MPI_T_ERR_PVAR_NO_ATOMIC
@*/
int MPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_READRESET);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_READRESET);

    /* Validate parameters */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_PVAR_SESSION(session, mpi_errno);
            MPIR_ERRTEST_PVAR_HANDLE(handle, mpi_errno);
            MPIR_ERRTEST_ARGNULL(buf, "buf", mpi_errno);
            if (handle == MPI_T_PVAR_ALL_HANDLES  || session != handle->session
                || !MPIR_T_pvar_is_oncestarted(handle))
            {
                mpi_errno = MPI_T_ERR_INVALID_HANDLE;
                goto fn_fail;
            }

            if (!MPIR_T_pvar_is_atomic(handle))
            {
                mpi_errno = MPI_T_ERR_PVAR_NO_ATOMIC;
                goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_readreset_impl(session, handle, buf);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_READRESET);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_readreset", "**mpi_t_pvar_readreset %p %p %p", session, handle, buf);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
