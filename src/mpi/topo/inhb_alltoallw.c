/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ineighbor_alltoallw */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ineighbor_alltoallw = PMPI_Ineighbor_alltoallw
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ineighbor_alltoallw  MPI_Ineighbor_alltoallw
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ineighbor_alltoallw as PMPI_Ineighbor_alltoallw
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                            const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                            void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[],
                            const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request) __attribute__((weak,alias("PMPI_Ineighbor_alltoallw")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ineighbor_alltoallw
#define MPI_Ineighbor_alltoallw PMPI_Ineighbor_alltoallw

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Ineighbor_alltoallw_default
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ineighbor_alltoallw_default(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int k,l;
    int *srcs, *dsts;
    MPIU_CHKLMEM_DECL(2);

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIU_CHKLMEM_MALLOC(srcs, int *, indegree*sizeof(int), mpi_errno, "srcs");
    MPIU_CHKLMEM_MALLOC(dsts, int *, outdegree*sizeof(int), mpi_errno, "dsts");
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb;
        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf + sdispls[k]);

        sb = ((char *)sendbuf) + sdispls[k];
        mpi_errno = MPID_Sched_send(sb, sendcounts[k], sendtypes[k], dsts[k], comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    for (l = 0; l < indegree; ++l) {
        char *rb;
        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf + rdispls[l]);

        rb = ((char *)recvbuf) + rdispls[l];
        mpi_errno = MPID_Sched_recv(rb, recvcounts[l], recvtypes[l], srcs[l], comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPID_SCHED_BARRIER(s);

fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ineighbor_alltoallw_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ineighbor_alltoallw_impl(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPID_Request *reqp = NULL;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Ineighbor_alltoallw != NULL);
    mpi_errno = comm_ptr->coll_fns->Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                                        recvbuf, recvcounts, rdispls, recvtypes,
                                                        comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, &reqp);
    if (reqp)
        *request = reqp->handle;
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ineighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ineighbor_alltoallw - Nonblocking version of MPI_Neighbor_alltoallw.

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
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INEIGHBOR_ALLTOALLW);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INEIGHBOR_ALLTOALLW);

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
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INEIGHBOR_ALLTOALLW);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ineighbor_alltoallw", "**mpi_ineighbor_alltoallw %p %p %p %p %p %p %p %p %C %p", sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
