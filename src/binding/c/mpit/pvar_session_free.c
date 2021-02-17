/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_session_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_session_free = PMPI_T_pvar_session_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_session_free  MPI_T_pvar_session_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_session_free as PMPI_T_pvar_session_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_session_free(MPI_T_pvar_session *session)
     __attribute__ ((weak, alias("PMPI_T_pvar_session_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_session_free
#define MPI_T_pvar_session_free PMPI_T_pvar_session_free

#endif

/*@
   MPI_T_pvar_session_free - Free an existing performance variable session

Input/Output Parameters:
. session - identifier of performance experiment session (handle)

Notes:
Calls to the MPI tool information interface can no longer be made
within the context of a session after it is freed. On a successful
return, MPI sets the session identifier to MPI_T_PVAR_SESSION_NULL.

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID_SESSION
.N MPI_T_ERR_NOT_INITIALIZED
@*/

int MPI_T_pvar_session_free(MPI_T_pvar_session *session)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_SESSION_FREE);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_SESSION_FREE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_PVAR_SESSION(*session);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_T_pvar_session_free_impl(session);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_SESSION_FREE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
