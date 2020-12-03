/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* -- Begin Profiling Symbol Block for routine MPI_Iscatterv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iscatterv = PMPI_Iscatterv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iscatterv  MPI_Iscatterv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iscatterv as PMPI_Iscatterv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Iscatterv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iscatterv
#define MPI_Iscatterv PMPI_Iscatterv
#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Iscatterv - Scatters a buffer in parts to all processes in a communicator
                in a nonblocking way

Input Parameters:
+ sendbuf - address of send buffer (significant only at root) (choice)
. sendcounts - non-negative integer array (of length group size) specifying the number of elements to send to each processor (significant only at root)
. displs - integer array (of length group size). Entry i specifies the displacement (relative to sendbuf) from which to take the outgoing data to process i (significant only at root)
. sendtype - data type of send buffer elements (significant only at root) (handle)
. recvcount - number of elements in receive buffer (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
. root - rank of sending process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                  MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ISCATTERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ISCATTERV);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            if (recvbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
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
            MPIR_Datatype *sendtype_ptr = NULL, *recvtype_ptr = NULL;
            int i, comm_size, rank;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
                rank = comm_ptr->rank;
                comm_size = comm_ptr->local_size;

                if (rank == root) {
                    for (i = 0; i < comm_size; i++) {
                        MPIR_ERRTEST_COUNT(sendcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    }
                    if (!HANDLE_IS_BUILTIN(sendtype)) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    for (i = 0; i < comm_size; i++) {
                        if (sendcounts[i] > 0) {
                            MPIR_ERRTEST_USERBUFFER(sendbuf, sendcounts[i], sendtype, mpi_errno);
                            break;
                        }
                    }
                    for (i = 0; i < comm_size; i++) {
                        if (sendcounts[i] > 0) {
                            MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcounts[i], mpi_errno);
                            break;
                        }
                    }
                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype &&
                        sendcounts[comm_ptr->rank] != 0 && recvcount != 0) {
                        int sendtype_size;
                        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(recvbuf,
                                                (char *) sendbuf +
                                                displs[comm_ptr->rank] * sendtype_size, mpi_errno);
                    }

                } else
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);

                if (recvbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (!HANDLE_IS_BUILTIN(recvtype)) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
                }
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
                if (root == MPI_ROOT) {
                    comm_size = comm_ptr->remote_size;
                    for (i = 0; i < comm_size; i++) {
                        MPIR_ERRTEST_COUNT(sendcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    }
                    if (!HANDLE_IS_BUILTIN(sendtype)) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    for (i = 0; i < comm_size; i++) {
                        if (sendcounts[i] > 0) {
                            MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(sendbuf, sendcounts[i], sendtype, mpi_errno);
                            break;
                        }
                    }
                } else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (!HANDLE_IS_BUILTIN(recvtype)) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
                }
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype,
                               root, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ISCATTERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_iscatterv", "**mpi_iscatterv %p %p %p %D %p %d %D %i %C %p",
                                 sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                 recvtype, root, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
