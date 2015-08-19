/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_idup */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_idup = PMPI_Comm_idup
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_idup  MPI_Comm_idup
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_idup as PMPI_Comm_idup
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request) __attribute__((weak,alias("PMPI_Comm_idup")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_idup
#define MPI_Comm_idup PMPI_Comm_idup

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_idup_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_idup_impl(MPID_Comm *comm_ptr, MPID_Comm **newcommp, MPID_Request **reqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Attribute *new_attributes = 0;

    /* Copy attributes, executing the attribute copy functions */
    /* This accesses the attribute dup function through the perprocess
       structure to prevent comm_dup from forcing the linking of the
       attribute functions.  The actual function is (by default)
       MPIR_Attr_dup_list
    */
    if (MPIR_Process.attr_dup) {
        mpi_errno = MPIR_Process.attr_dup(comm_ptr->handle,
                                          comm_ptr->attributes,
                                          &new_attributes);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    mpi_errno = MPIR_Comm_copy_data(comm_ptr, newcommp);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    (*newcommp)->attributes = new_attributes;

    /* We now have a mostly-valid new communicator, so begin the process of
     * allocating a context ID to use for actual communication */
    if (comm_ptr->comm_kind == MPID_INTERCOMM) {
        mpi_errno = MPIR_Get_intercomm_contextid_nonblock(comm_ptr, *newcommp, reqp);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIR_Get_contextid_nonblock(comm_ptr, *newcommp, reqp);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Comm_idup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Comm_idup - nonblocking communicator duplication

Input Parameters:
. comm - communicator (handle)

Output Parameters:
+ newcomm - copy of comm (handle)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *newcomm_ptr = NULL;
    MPID_Request *dreq = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_IDUP);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_IDUP);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    *request = MPI_REQUEST_NULL;
    *newcomm = MPI_COMM_NULL;

    mpi_errno = MPIR_Comm_idup_impl(comm_ptr, &newcomm_ptr, &dreq);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* NOTE: this is a publication for most of the comm, but the context ID
     * won't be valid yet, so we must "republish" relative to the request
     * handle at request completion time. */
    MPID_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    *request = dreq->handle;

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_IDUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_comm_idup", "**mpi_comm_idup %C %p %p", comm, newcomm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
