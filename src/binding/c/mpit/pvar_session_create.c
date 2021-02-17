/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* -- THIS FILE IS AUTO-GENERATED -- */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_session_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_session_create = PMPI_T_pvar_session_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_session_create  MPI_T_pvar_session_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_session_create as PMPI_T_pvar_session_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_session_create(MPI_T_pvar_session *session)
     __attribute__ ((weak, alias("PMPI_T_pvar_session_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_session_create
#define MPI_T_pvar_session_create PMPI_T_pvar_session_create

#endif

/*@
   MPI_T_pvar_session_create - Create a new session for accessing performance variables

Output Parameters:
. session - identifier of performance session (handle)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS

.N MPI_T_ERR_INVALID
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_OUT_OF_SESSIONS
@*/

int MPI_T_pvar_session_create(MPI_T_pvar_session *session)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_SESSION_CREATE);

    MPIT_ERRTEST_MPIT_INITIALIZED();

    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_SESSION_CREATE);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIT_ERRTEST_ARGNULL(session);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    mpi_errno = MPIR_T_pvar_session_create_impl(session);
    if (mpi_errno) {
        goto fn_fail;
    }
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_SESSION_CREATE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
