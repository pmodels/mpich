/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Neighbor_alltoallw */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Neighbor_alltoallw = PMPI_Neighbor_alltoallw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Neighbor_alltoallw  MPI_Neighbor_alltoallw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Neighbor_alltoallw as PMPI_Neighbor_alltoallw
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[],
                           const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                           const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm) __attribute__((weak,alias("PMPI_Neighbor_alltoallw")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Neighbor_alltoallw
#define MPI_Neighbor_alltoallw PMPI_Neighbor_alltoallw

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_alltoallw_default
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_alltoallw_default(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Request req;

    /* just call the nonblocking version and wait on it */
    mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, &req);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Wait_impl(&req, MPI_STATUS_IGNORE);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Neighbor_alltoallw_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Neighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Neighbor_alltoallw != NULL);
    mpi_errno = comm_ptr->coll_fns->Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                       recvbuf, recvcounts, rdispls, recvtypes,
                                                       comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Neighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Neighbor_alltoallw - Like MPI_Neighbor_alltoallv but it allows one to send
and receive with different types to and from each neighbor.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
. sdispls - integer array (of length outdegree).  Entry j specifies the displacement in bytes (relative to sendbuf) from which to take the outgoing data destined for neighbor j (array of integers)
. sendtypes - array of datatypes (of length outdegree).  Entry j specifies the type of data to send to neighbor j (array of handles)
. recvcounts - non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
. rdispls - integer array (of length indegree).  Entry i specifies the displacement in bytes (relative to recvbuf) at which to place the incoming data from neighbor i (array of integers).
. recvtypes - array of datatypes (of length indegree).  Entry i specifies the type of data received from neighbor i (array of handles).
- comm - communicator with topology structure (handle)

Output Parameters:
. recvbuf - starting address of the receive buffer (choice)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_NEIGHBOR_ALLTOALLW);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_NEIGHBOR_ALLTOALLW);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
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
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Neighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_NEIGHBOR_ALLTOALLW);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_neighbor_alltoallw", "**mpi_neighbor_alltoallw %p %p %p %p %p %p %p %p %C", sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
