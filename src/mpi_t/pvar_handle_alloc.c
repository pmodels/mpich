/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpl_utlist.h"

/* -- Begin Profiling Symbol Block for routine MPIX_T_pvar_handle_alloc */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_T_pvar_handle_alloc = PMPIX_T_pvar_handle_alloc
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_T_pvar_handle_alloc  MPIX_T_pvar_handle_alloc
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_T_pvar_handle_alloc as PMPIX_T_pvar_handle_alloc
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_T_pvar_handle_alloc
#define MPIX_T_pvar_handle_alloc PMPIX_T_pvar_handle_alloc

/* define storage for the ALL_HANDLES constant */
struct MPIR_T_pvar_handle MPIR_T_pvar_all_handles_obj = {NULL, NULL, NULL, NULL, -1, -1, NULL, NULL};
struct MPIR_T_pvar_handle * const MPIX_T_PVAR_ALL_HANDLES = &MPIR_T_pvar_all_handles_obj;

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_T_pvar_handle_alloc_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_T_pvar_handle_alloc_impl(MPIX_T_pvar_session session, int pvar_index, void *obj_handle, MPIX_T_pvar_handle *handle, int *count)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPIR_T_pvar_info *info = NULL;
    MPIU_CHKPMEM_DECL(1);

    *handle = MPIX_T_PVAR_HANDLE_NULL;
    *count = 0xdeadbeef;

    mpi_errno = MPIR_T_get_pvar_info_by_idx(pvar_index, &info);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_CHKPMEM_MALLOC(*handle, MPIX_T_pvar_handle, sizeof(**handle), mpi_errno, "performance var handle");

    /* setup the common fields */
    (*handle)->session = session;
    (*handle)->info = info;

    /* now some defaults that the create_fn might override */
    (*handle)->bytes = -1;
    (*handle)->vtable = NULL;
    (*handle)->handle_state = NULL;
    (*handle)->free_handle_state = FALSE;

    /* dispatch particulars of the handle to the creator function */
    mpi_errno = info->create_fn(obj_handle, (*handle), count);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPL_DL_APPEND(session->hlist, *handle);

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_T_pvar_handle_alloc
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_T_pvar_handle_alloc - XXX description here

Input Parameters:
+ session - identifier of performance experiment session (handle)
. pvar_index - index of performance variable for which handle is to be allocated (integer)
- obj_handle - reference to a handle of the MPI object to which this variable is supposed to be bound (pointer)

Output Parameters:
+ handle - allocated handle (handle)
- count - number of elements used to represent this variable (integer)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_T_pvar_handle_alloc(MPIX_T_pvar_session session, int pvar_index, void *obj_handle, MPIX_T_pvar_handle *handle, int *count)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_T_PVAR_HANDLE_ALLOC);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_T_PVAR_HANDLE_ALLOC);

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
            MPIR_ERRTEST_ARGNULL(count, "count", mpi_errno);
            MPIR_ERRTEST_ARGNULL(handle, "handle", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_T_pvar_handle_alloc_impl(session, pvar_index, obj_handle, handle, count);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_T_PVAR_HANDLE_ALLOC);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_t_pvar_handle_alloc", "**mpix_t_pvar_handle_alloc %p %d %p %p %p", session, pvar_index, obj_handle, handle, count);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
