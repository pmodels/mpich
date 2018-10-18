/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_session_create */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_session_create = PMPI_T_pvar_session_create
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_session_create  MPI_T_pvar_session_create
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_session_create as PMPI_T_pvar_session_create
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_session_create(MPI_T_pvar_session * session)
    __attribute__ ((weak, alias("PMPI_T_pvar_session_create")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_session_create
#define MPI_T_pvar_session_create PMPI_T_pvar_session_create

/* any non-MPI functions go here, especially non-static ones */

int MPIR_T_pvar_session_create_impl(MPI_T_pvar_session * session)
{
    int mpi_errno = MPI_SUCCESS;
    MPIT_CHKPMEM_DECL(1);

    *session = MPI_T_PVAR_SESSION_NULL;

    MPIT_CHKPMEM_MALLOC(*session, MPI_T_pvar_session, sizeof(**session), MPL_MEM_MPIT);

    /* essential for utlist to work */
    (*session)->hlist = NULL;
    (*session)->kind = MPIR_T_PVAR_SESSION;

    MPIT_CHKPMEM_COMMIT();
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIT_CHKPMEM_REAP();
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_T_pvar_session_create - Create a new session for accessing performance variables

Output Parameters:
. session - identifier of performance session (handle)

.N ThreadSafe

.N Errors
.N MPI_SUCCESS
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_OUT_OF_SESSIONS
@*/
int MPI_T_pvar_session_create(MPI_T_pvar_session * session)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_T_PVAR_SESSION_CREATE);
    MPIT_ERRTEST_MPIT_INITIALIZED();
    MPIR_T_THREAD_CS_ENTER();
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_T_PVAR_SESSION_CREATE);

    /* Validate parameters, especially handles needing to be converted */
    MPIT_ERRTEST_ARGNULL(session);

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_session_create_impl(session);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_T_PVAR_SESSION_CREATE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
