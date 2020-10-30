/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* -- Begin Profiling Symbol Block for routine MPI_Ialltoallw */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ialltoallw = PMPI_Ialltoallw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ialltoallw  MPI_Ialltoallw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ialltoallw as PMPI_Ialltoallw
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request * request) __attribute__ ((weak, alias("PMPI_Ialltoallw")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ialltoallw
#define MPI_Ialltoallw PMPI_Ialltoallw
#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ialltoallw - Nonblocking generalized all-to-all communication allowing
   different datatypes, counts, and displacements for each partner

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length group size) specifying the number of elements to send to each processor
. sdispls - integer array (of length group size). Entry j specifies the displacement relative to sendbuf from which to take the outgoing data destined for process j
. sendtypes - array of datatypes (of length group size). Entry j specifies the type of data to send to process j (array of handles)
. recvcounts - non-negative integer array (of length group size) specifying the number of elements that can be received from each processor
. rdispls - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtypes - array of datatypes (of length group size). Entry i specifies the type of data received from process i (array of handles)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                   const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm,
                   MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLTOALLW);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLTOALLW);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_ARGNULL(sendcounts, "sendcounts", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sdispls, "sdispls", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sendtypes, "sendtypes", mpi_errno);

                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                    sendcounts == recvcounts && sendtypes == recvtypes)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(recvcounts, "recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(rdispls, "rdispls", mpi_errno);
            MPIR_ERRTEST_ARGNULL(recvtypes, "recvtypes", mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM && sendbuf == MPI_IN_PLACE) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sendbuf_inplace");
            }
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                recvcounts, rdispls, recvtypes, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLTOALLW);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ialltoallw",
                                 "**mpi_ialltoallw %p %p %p %p %p %p %p %p %C %p", sendbuf,
                                 sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls,
                                 recvtypes, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
