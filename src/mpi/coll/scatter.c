/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"


/* -- Begin Profiling Symbol Block for routine MPI_Scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Scatter = PMPI_Scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Scatter  MPI_Scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Scatter as PMPI_Scatter
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Scatter")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Scatter
#define MPI_Scatter PMPI_Scatter
#endif

/*@

MPI_Scatter - Sends data from one process to all other processes in a
communicator

Input Parameters:
+ sendbuf - address of send buffer (choice, significant
only at 'root')
. sendcount - number of elements sent to each process
(integer, significant only at 'root')
. sendtype - data type of send buffer elements (significant only at 'root')
(handle)
. recvcount - number of elements in receive buffer (integer)
. recvtype - data type of receive buffer elements (handle)
. root - rank of sending process (integer)
- comm - communicator (handle)

Output Parameters:
. recvbuf - address of receive buffer (choice)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_SCATTER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_SCATTER);

    /* Validate parameters, especially handles needing to be converted */
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

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_Datatype *sendtype_ptr = NULL, *recvtype_ptr = NULL;
            int rank;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                rank = comm_ptr->rank;
                if (rank == root) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (!HANDLE_IS_BUILTIN(sendtype)) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);

                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcount == recvcount &&
                        recvcount != 0) {
                        int sendtype_size;
                        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(recvbuf,
                                                (char *) sendbuf +
                                                comm_ptr->rank * sendcount * sendtype_size,
                                                mpi_errno);
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
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (!HANDLE_IS_BUILTIN(sendtype)) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                        MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                        if (mpi_errno != MPI_SUCCESS)
                            goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
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

    mpi_errno = MPIR_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
                             comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_SCATTER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_scatter", "**mpi_scatter %p %d %D %p %d %D %i %C", sendbuf,
                                 sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
