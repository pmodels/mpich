/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Sendrecv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Sendrecv  MPI_Sendrecv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Sendrecv as PMPI_Sendrecv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                 int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status * status)
    __attribute__ ((weak, alias("PMPI_Sendrecv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Sendrecv
#define MPI_Sendrecv PMPI_Sendrecv

#endif

/*@
    MPI_Sendrecv - Sends and receives a message

Input Parameters:
+ sendbuf - initial address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - type of elements in send buffer (handle)
. dest - rank of destination (integer)
. sendtag - send tag (integer)
. recvcount - number of elements in receive buffer (integer)
. recvtype - type of elements in receive buffer (handle)
. source - rank of source (integer)
. recvtag - receive tag (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - initial address of receive buffer (choice)
- status - status object (Status).  This refers to the receive operation.

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 int dest, int sendtag,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int source, int recvtag, MPI_Comm comm, MPI_Status * status)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_SENDRECV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_PT2PT_ENTER_BOTH(MPID_STATE_MPI_SENDRECV);

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

    /* Convert handles to MPI objects. */
    MPIR_Comm_get_ptr(comm, comm_ptr);

#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate communicator */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno)
                goto fn_fail;

            /* Validate count */
            MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);

            /* Validate status (status_ignore is not the same as null) */
            MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);

            /* Validate tags */
            MPIR_ERRTEST_SEND_TAG(sendtag, mpi_errno);
            MPIR_ERRTEST_RECV_TAG(recvtag, mpi_errno);

            /* Validate source and destination */
            if (comm_ptr) {
                MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
                MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
            }

            /* Validate datatype handles */
            MPIR_ERRTEST_DATATYPE(sendtype, "datatype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "datatype", mpi_errno);

            /* Validate datatype objects */
            if (!HANDLE_IS_BUILTIN(sendtype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(sendtype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }
            if (!HANDLE_IS_BUILTIN(recvtype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(recvtype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno)
                    goto fn_fail;
            }

            /* Validate buffers */
            MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Sendrecv_impl(sendbuf, sendcount, sendtype, dest, sendtag,
                                   recvbuf, recvcount, recvtype, source, recvtag, comm_ptr, status);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_PT2PT_EXIT_BOTH(MPID_STATE_MPI_SENDRECV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_sendrecv",
                                 "**mpi_sendrecv %p %d %D %i %t %p %d %D %i %t %C %p", sendbuf,
                                 sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype,
                                 source, recvtag, comm, status);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
