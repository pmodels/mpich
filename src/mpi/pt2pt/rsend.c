/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Rsend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Rsend = PMPI_Rsend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Rsend  MPI_Rsend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Rsend as PMPI_Rsend
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm) __attribute__ ((weak, alias("PMPI_Rsend")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Rsend
#define MPI_Rsend PMPI_Rsend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Rsend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Rsend - Blocking ready send

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (nonnegative integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_RSEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER_FRONT(MPID_STATE_MPI_RSEND);

    /* Validate handle parameters needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters if error checking is enabled */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno)
                goto fn_fail;

            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
            MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);

            /* Validate datatype handle */
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* Validate datatype object */
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            /* Validate buffer */
            MPIR_ERRTEST_USERBUFFER(buf, count, datatype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    mpi_errno = MPID_Rsend(buf, count, datatype, dest, tag, comm_ptr,
                           MPIR_CONTEXT_INTRA_PT2PT, &request_ptr);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    if (request_ptr == NULL) {
        goto fn_exit;
    }

    /* If a request was returned, then we need to block until the request
     * is complete */
    mpi_errno = MPID_Wait(request_ptr, MPI_STATUS_IGNORE);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = request_ptr->status.MPI_ERROR;
    MPIR_Request_free(request_ptr);

    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT(MPID_STATE_MPI_RSEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_rsend", "**mpi_rsend %p %d %D %i %t %C", buf, count,
                                 datatype, dest, tag, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
