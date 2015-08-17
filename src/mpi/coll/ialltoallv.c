/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Ialltoallv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ialltoallv = PMPI_Ialltoallv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ialltoallv  MPI_Ialltoallv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ialltoallv as PMPI_Ialltoallv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm,
                   MPI_Request *request)
                   __attribute__((weak,alias("PMPI_Ialltoallv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ialltoallv
#define MPI_Ialltoallv PMPI_Ialltoallv

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallv_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallv_intra(const void *sendbuf, const int sendcounts[], const int sdispls[],
                          MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                          const int rdispls[], MPI_Datatype recvtype, MPID_Comm *comm_ptr,
                          MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    int i, j;
    int ii, ss, bblock;
    MPI_Aint send_extent, recv_extent, sendtype_size, recvtype_size;
    int dst, rank;
    MPIR_SCHED_CHKPMEM_DECL(1);

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* Get extent and size of recvtype, don't look at sendtype for MPI_IN_PLACE */
    MPID_Datatype_get_extent_macro(recvtype, recv_extent);
    MPID_Datatype_get_size_macro(recvtype, recvtype_size);

    if (sendbuf == MPI_IN_PLACE) {
        int max_count;
        void *tmp_buf = NULL;

        /* The regular MPI_Alltoallv handles MPI_IN_PLACE using pairwise
         * sendrecv_replace calls.  We don't have a sendrecv_replace, so just
         * malloc the maximum of the counts array entries and then perform the
         * pairwise exchanges manually with schedule barriers instead.
         *
         * Because of this approach all processes must agree on the global
         * schedule of "sendrecv_replace" operations to avoid deadlock.
         *
         * This keeps with the spirit of the MPI-2.2 standard, which is to
         * conserve memory when using MPI_IN_PLACE for these routines.
         * Something like MADRE would probably generate a more optimal
         * algorithm. */
        max_count = 0;
        for (i = 0; i < comm_size; ++i) {
            max_count = MPIU_MAX(max_count, recvcounts[i]);
        }

        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, max_count*recv_extent, mpi_errno, "Ialltoallv tmp_buf");

        for (i = 0; i < comm_size; ++i) {
            /* start inner loop at i to avoid re-exchanging data */
            for (j = i; j < comm_size; ++j) {
                if (rank == i && rank == j) {
                    /* no need to "sendrecv_replace" for ourselves */
                }
                else if (rank == i || rank == j) {
                    if (rank == i)
                        dst = j;
                    else
                        dst = i;

                    mpi_errno = MPID_Sched_send(((char *)recvbuf + rdispls[dst]*recv_extent),
                                                recvcounts[dst], recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPID_Sched_recv(tmp_buf, recvcounts[dst], recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPID_SCHED_BARRIER(s);

                    mpi_errno = MPID_Sched_copy(tmp_buf, recvcounts[dst], recvtype,
                                                ((char *)recvbuf + rdispls[dst]*recv_extent),
                                                recvcounts[dst], recvtype, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPID_SCHED_BARRIER(s);
                }
            }
        }

        MPID_SCHED_BARRIER(s);
    }
    else {
        bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
        if (bblock == 0)
            bblock = comm_size;

        /* get size/extent for sendtype */
        MPID_Datatype_get_extent_macro(sendtype, send_extent);
        MPID_Datatype_get_size_macro(sendtype, sendtype_size);

        /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
        for (ii=0; ii<comm_size; ii+=bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;

            /* do the communication -- post ss sends and receives: */
            for (i=0; i < ss; i++) {
                dst = (rank+i+ii) % comm_size;
                if (recvcounts[dst] && recvtype_size) {
                    MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                                     rdispls[dst]*recv_extent);
                    mpi_errno = MPID_Sched_recv((char *)recvbuf+rdispls[dst]*recv_extent,
                                                recvcounts[dst], recvtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }

            for (i=0; i < ss; i++) {
                dst = (rank-i-ii+comm_size) % comm_size;
                if (sendcounts[dst] && sendtype_size) {
                    MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                                     sdispls[dst]*send_extent);
                    mpi_errno = MPID_Sched_send((char *)sendbuf+sdispls[dst]*send_extent,
                                                sendcounts[dst], sendtype, dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }

            /* force our block of sends/recvs to complete before starting the next block */
            MPID_SCHED_BARRIER(s);
        }
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallv_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallv_inter(const void *sendbuf, const int sendcounts[], const int sdispls[],
                          MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                          const int rdispls[], MPI_Datatype recvtype, MPID_Comm *comm_ptr,
                          MPID_Sched_t s)
{
/* Intercommunicator alltoallv. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoallv. Since the
   local and remote groups can be of different
   sizes, we first compute the max of local_group_size,
   remote_group_size. At step i, 0 <= i < max_size, each process
   receives from src = (rank - i + max_size) % max_size if src <
   remote_size, and sends to dst = (rank + i) % max_size if dst <
   remote_size.
*/
    int mpi_errno = MPI_SUCCESS;
    int local_size, remote_size, max_size, i;
    MPI_Aint   send_extent, recv_extent, sendtype_size, recvtype_size;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Get extent of send and recv types */
    MPID_Datatype_get_extent_macro(sendtype, send_extent);
    MPID_Datatype_get_extent_macro(recvtype, recv_extent);
    MPID_Datatype_get_size_macro(sendtype, sendtype_size);
    MPID_Datatype_get_size_macro(recvtype, recvtype_size);

    /* Use pairwise exchange algorithm. */
    max_size = MPIR_MAX(local_size, remote_size);
    for (i=0; i<max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
            recvcount = 0;
        }
        else {
            MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                             rdispls[src]*recv_extent);
            recvaddr = (char *)recvbuf + rdispls[src]*recv_extent;
            recvcount = recvcounts[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
        }
        else {
            MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                             sdispls[dst]*send_extent);
            sendaddr = (char *)sendbuf + sdispls[dst]*send_extent;
            sendcount = sendcounts[dst];
        }

        if (sendcount * sendtype_size == 0)
            dst = MPI_PROC_NULL;
        if (recvcount * recvtype_size == 0)
            src = MPI_PROC_NULL;

        mpi_errno = MPID_Sched_send(sendaddr, sendcount, sendtype, dst, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPID_Sched_recv(recvaddr, recvcount, recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPID_Sched_barrier(s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallv_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                         MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                         const int rdispls[], MPI_Datatype recvtype, MPID_Comm *comm_ptr,
                         MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *reqp = NULL;
    int tag = -1;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    if (comm_ptr->coll_fns->Ialltoallv_req != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->coll_fns->Ialltoallv_req(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr, &reqp);
        if (reqp) {
            *request = reqp->handle;
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            goto fn_exit;
        }
        /* --END USEREXTENSION-- */
    }

    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Ialltoallv_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Ialltoallv_sched(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr, s);
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
#define FUNCNAME MPI_Ialltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ialltoallv - Sends data from all to all processes in a nonblocking way;
   each process may send a different amount of data and provide displacements
   for the input and output data.

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcounts - non-negative integer array (of length group size) specifying the number of elements to send to each processor
. sdispls - integer array (of length group size). Entry j specifies the displacement relative to sendbuf from which to take the outgoing data destined for process j
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) specifying the number of elements that can be received from each processor
. rdispls - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IALLTOALLV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_IALLTOALLV);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (sendbuf != MPI_IN_PLACE)
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
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_ARGNULL(sendcounts,"sendcounts", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sdispls,"sdispls", mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPID_Datatype *sendtype_ptr = NULL;
                    MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPID_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    MPID_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                }
            }

            MPIR_ERRTEST_ARGNULL(recvcounts,"recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(rdispls,"rdispls", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *recvtype_ptr = NULL;
                MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPID_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            if (comm_ptr->comm_kind == MPID_INTRACOMM &&
                    sendbuf != MPI_IN_PLACE &&
                    sendcounts == recvcounts &&
                    sendtype == recvtype)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf,recvbuf,mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_IALLTOALLV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ialltoallv", "**mpi_ialltoallv %p %p %p %D %p %p %p %D %C %p", sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
