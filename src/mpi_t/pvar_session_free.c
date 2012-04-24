/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_pvar_session_free */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_pvar_session_free = PMPIX_T_pvar_session_free
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_pvar_session_free  MPIX_T_pvar_session_free
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_pvar_session_free as PMPIX_T_pvar_session_free
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_pvar_session_free
#define MPIX_T_pvar_session_free PMPIX_T_pvar_session_free

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_session_free_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_pvar_session_free_impl(MPIX_T_pvar_session *session)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_T_pvar_handle *hnd, *tmp;

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
    *session = MPIX_T_PVAR_SESSION_NULL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_pvar_session_free
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_pvar_session_free - XXX description here

Input/Output Parameters:
. session - identifier of performance experiment session (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_pvar_session_free(MPIX_T_pvar_session *session)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_PVAR_SESSION_FREE);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_PVAR_SESSION_FREE);

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
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_session_free_impl(session);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_PVAR_SESSION_FREE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_pvar_session_free", "**mpix_t_pvar_session_free %p", session);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
