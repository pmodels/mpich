/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Neighbor_allgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Neighbor_allgather = PMPI_Neighbor_allgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Neighbor_allgather  MPI_Neighbor_allgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Neighbor_allgather as PMPI_Neighbor_allgather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
                           __attribute__((weak,alias("PMPI_Neighbor_allgather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Neighbor_allgather
#define MPI_Neighbor_allgather PMPI_Neighbor_allgather

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_allgather_default
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_allgather_default(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Request req;

    /* just call the nonblocking version and wait on it */
    mpi_errno = MPIR_Ineighbor_allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr, &req);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Wait_impl(&req, MPI_STATUS_IGNORE);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_allgather_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Neighbor_allgather != NULL);
    mpi_errno = comm_ptr->coll_fns->Neighbor_allgather(sendbuf, sendcount, sendtype,
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
#define FUNCNAME MPI_Neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Neighbor_allgather - In this function, each process i gathers data items
from each process j if an edge (j,i) exists in the topology graph, and each
process i sends the same data items to all processes j where an edge (i,j)
exists. The send buffer is sent to each neighboring process and the l-th block
in the receive buffer is received from the l-th neighbor.

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
int MPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_NEIGHBOR_ALLGATHER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_NEIGHBOR_ALLGATHER);

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

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_NEIGHBOR_ALLGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_neighbor_allgather", "**mpi_neighbor_allgather %p %d %D %p %d %D %C", sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
