/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Neighbor_alltoall */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Neighbor_alltoall = PMPI_Neighbor_alltoall
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Neighbor_alltoall  MPI_Neighbor_alltoall
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Neighbor_alltoall as PMPI_Neighbor_alltoall
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                          __attribute__((weak,alias("PMPI_Neighbor_alltoall")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Neighbor_alltoall
#define MPI_Neighbor_alltoall PMPI_Neighbor_alltoall

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_alltoall_default
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_alltoall_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Request req;
    MPIR_Request *req_ptr;
    int active_flag;

    /* just call the nonblocking version and wait on it */
    mpi_errno = MPID_Ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             comm_ptr, &req);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_Assert(req != MPI_REQUEST_NULL);
    MPIR_Request_get_ptr(req, req_ptr);
    mpi_errno = MPIR_Wait_impl(req_ptr, MPI_STATUS_IGNORE);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Request_complete(&req, req_ptr, MPI_STATUS_IGNORE, &active_flag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_alltoall_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_alltoall_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPIR_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm_ptr->coll_fns != NULL);
    MPIR_Assert(comm_ptr->coll_fns->Neighbor_alltoall != NULL);
    mpi_errno = comm_ptr->coll_fns->Neighbor_alltoall(sendbuf, sendcount, sendtype,
                                                      recvbuf, recvcount, recvtype,
                                                      comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Neighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Neighbor_alltoall - In this function, each process i receives data items
from each process j if an edge (j,i) exists in the topology graph or Cartesian
topology.  Similarly, each process i sends data items to all processes j where an
edge (i,j) exists. This call is more general than MPI_NEIGHBOR_ALLGATHER in that
different data items can be sent to each neighbor. The k-th block in send buffer
is sent to the k-th neighboring process and the l-th block in the receive buffer
is received from the l-th neighbor.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements sent to each neighbor (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from each neighbor (non-negative integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - starting address of the receive buffer (choice)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_NEIGHBOR_ALLTOALL);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_NEIGHBOR_ALLTOALL);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *sendtype_ptr = NULL;
                MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
            }

            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
            }

            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_NEIGHBOR_ALLTOALL);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_neighbor_alltoall", "**mpi_neighbor_alltoall %p %d %D %p %d %D %C", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
