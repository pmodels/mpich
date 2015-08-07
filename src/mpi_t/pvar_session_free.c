/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"

/* -- Begin Profiling Symbol Block for routine MPI_T_pvar_session_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_T_pvar_session_free = PMPI_T_pvar_session_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_T_pvar_session_free  MPI_T_pvar_session_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_T_pvar_session_free as PMPI_T_pvar_session_free
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_T_pvar_session_free(MPI_T_pvar_session *session) __attribute__((weak,alias("PMPI_T_pvar_session_free")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_T_pvar_session_free
#define MPI_T_pvar_session_free PMPI_T_pvar_session_free

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_session_free_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_T_pvar_session_free_impl(MPI_T_pvar_session *session)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_T_pvar_handle_t *hnd, *tmp;

    /* The draft standard is a bit unclear about the behavior of this function
     * w.r.t. outstanding pvar handles.  A conservative (from the implementors
     * viewpoint) interpretation implies that this routine should free all
     * outstanding handles attached to this session.  A more relaxed
     * interpretation puts that burden on the user.  We choose the conservative
     * view.*/
    MPL_DL_FOREACH_SAFE((*session)->hlist, hnd, tmp) {
        MPL_DL_DELETE((*session)->hlist, hnd);
        MPIU_Free(hnd);
    }
    MPIU_Free(*session);
    *session = MPI_T_PVAR_SESSION_NULL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_T_pvar_session_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
.N MPI_T_ERR_NOT_INITIALIZED
.N MPI_T_ERR_INVALID_SESSION
@*/
int MPI_T_pvar_session_free(MPI_T_pvar_session *session)
{
    int mpi_errno = MPI_SUCCESS;

    MPID_MPI_STATE_DECL(MPID_STATE_MPI_T_PVAR_SESSION_FREE);
    MPIR_ERRTEST_MPIT_INITIALIZED(mpi_errno);
    MPIR_T_THREAD_CS_ENTER();
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_T_PVAR_SESSION_FREE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_ARGNULL(session, "session", mpi_errno);
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_session_free_impl(session);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_T_PVAR_SESSION_FREE);
    MPIR_T_THREAD_CS_EXIT();
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_t_pvar_session_free", "**mpi_t_pvar_session_free %p", session);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
