/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_init_thread */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_init_thread = PMPIX_T_init_thread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_init_thread  MPIX_T_init_thread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_init_thread as PMPIX_T_init_thread
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_init_thread
#define MPIX_T_init_thread PMPIX_T_init_thread

/* any non-MPI functions go here, especially non-static ones */

/* a counter that keeps track of the relative balance of calls to
 * MPIX_T_init_thread and MPIX_T_finalize */
int MPIR_T_init_balance = 0;

/* returns true iff the MPIX_T_ interface is currently initialized */
int MPIR_T_is_initialized(void)
{
    return (MPIR_T_init_balance > 0);
}

#undef FUNCNAME
#define FUNCNAME MPIR_T_init_thread_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_init_thread_impl(int required, int *provided)
{
    int mpi_errno = MPI_SUCCESS;

    /* TODO deal with required/provided and threading initialization */
    /* always claim SINGLE for now, since it's a lot of work to separately
     * initialize all of the threading code without initializing the whole MPI
     * library right now */
    *provided = MPI_THREAD_SINGLE;

    ++MPIR_T_init_balance;
    if (MPIR_T_init_balance == 1) {
        /* _init_params is idempotent, so it's OK if we call it after or before
         * MPI_Init does */
        mpi_errno = MPIR_Param_init_params();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_init_thread
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_init_thread - XXX description here

Input Parameters:
. required - desired level of thread support (integer)

Output Parameters:
. provided - provided level of thread support (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_init_thread(int required, int *provided)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_INIT_THREAD);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_INIT_THREAD);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_ARGNULL(provided, "provided", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_init_thread_impl(required, provided);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_INIT_THREAD);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_init_thread", "**mpix_t_init_thread %d %d", required, provided);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
