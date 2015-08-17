/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "topo.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ineighbor_alltoallv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ineighbor_alltoallv = PMPI_Ineighbor_alltoallv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ineighbor_alltoallv  MPI_Ineighbor_alltoallv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ineighbor_alltoallv as PMPI_Ineighbor_alltoallv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                            MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                            const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                            MPI_Request *request)
                            __attribute__((weak,alias("PMPI_Ineighbor_alltoallv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ineighbor_alltoallv
#define MPI_Ineighbor_alltoallv PMPI_Ineighbor_alltoallv

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Ineighbor_alltoallv_default
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ineighbor_alltoallv_default(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int i, k,l;
    int *srcs, *dsts;
    int comm_size;
    MPI_Aint sendtype_extent, recvtype_extent;
    MPIU_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;

    MPID_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPID_Datatype_get_extent_macro(recvtype, recvtype_extent);

    for (i = 0; i < comm_size; ++i) {
        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                         (sdispls[i] * sendtype_extent));
        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         (rdispls[i] * recvtype_extent));
    }

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIU_CHKLMEM_MALLOC(srcs, int *, indegree*sizeof(int), mpi_errno, "srcs");
    MPIU_CHKLMEM_MALLOC(dsts, int *, outdegree*sizeof(int), mpi_errno, "dsts");
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb = ((char *)sendbuf) + sdispls[k] * sendtype_extent;
        mpi_errno = MPID_Sched_send(sb, sendcounts[k], sendtype, dsts[k], comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    for (l = 0; l < indegree; ++l) {
        char *rb = ((char *)recvbuf) + rdispls[l] * recvtype_extent;
        mpi_errno = MPID_Sched_recv(rb, recvcounts[l], recvtype, srcs[l], comm_ptr, s);
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
#define FUNCNAME MPIR_Ineighbor_alltoallv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ineighbor_alltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPID_Comm *comm_ptr, MPI_Request *request)
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
    MPIU_Assert(comm_ptr->coll_fns->Ineighbor_alltoallv != NULL);
    mpi_errno = comm_ptr->coll_fns->Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                                        recvbuf, recvcounts, rdispls, recvtype,
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
#define FUNCNAME MPI_Ineighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ineighbor_alltoallv - Nonblocking version of MPI_Neighbor_alltoallv.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length outdegree) specifying the number of elements to send to each neighbor
. sdispls - integer array (of length outdegree).  Entry j specifies the displacement (relative to sendbuf) from which to send the outgoing data to neighbor j
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length indegree) specifying the number of elements that are received from each neighbor
. rdispls - integer array (of length indegree).  Entry i specifies the displacement (relative to recvbuf) at which to place the incoming data from neighbor i.
. recvtype - data type of receive buffer elements (handle)
- comm - communicator with topology structure (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_INEIGHBOR_ALLTOALLV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_INEIGHBOR_ALLTOALLV);

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
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *recvtype_ptr = NULL;
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_INEIGHBOR_ALLTOALLV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ineighbor_alltoallv", "**mpi_ineighbor_alltoallv %p %p %p %D %p %p %p %D %C %p", sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
