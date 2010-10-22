/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 110 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Iallgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Iallgather = PMPIX_Iallgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Iallgather  MPIX_Iallgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Iallgather as PMPIX_Iallgather
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Iallgather
#define MPIX_Iallgather PMPIX_Iallgather

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgather_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Iallgather_impl(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Iallgather != NULL) {
        mpi_errno = comm_ptr->coll_fns->Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, request);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "default version of MPIX_Iallgather not yet implemented");
        MPIU_Assertp(0); /* never get here */
        /* TODO implement the following functions and uncomment this code: */
#if 0
        if (comm_ptr->comm_kind == MPID_INTRACOMM) {
            /* intracommunicator */
            mpi_errno = MPIR_Iallgather_intra(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else {
            /* intercommunicator */
            mpi_errno = MPIR_Iallgather_inter(sendbuf, recvbuf, recvcount, datatype, op, comm_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
#endif
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Allreduce
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_Iallgather - XXX description here

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements in receive buffer (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Iallgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_IALLGATHER);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_IALLGATHER);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
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
            if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *sendtype_ptr = NULL;
                MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPID_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
            }

            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *recvtype_ptr = NULL;
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
            }

            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, request);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_IALLGATHER);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_iallgather", "**mpix_iallgather %p %d %D %p %d %D %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
