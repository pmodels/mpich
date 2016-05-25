/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
                   MPI_Request *request) __attribute__((weak,alias("PMPI_Ialltoallw")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ialltoallw
#define MPI_Ialltoallw PMPI_Ialltoallw

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of alltoallw. The algorithm is:

   Algorithm: MPI_Alltoallw

   Since each process sends/receives different amounts of data to
   every other process, we don't know the total message size for all
   processes without additional communication. Therefore we simply use
   the "middle of the road" isend/irecv algorithm that works
   reasonably well in all cases.

   We post all irecvs and isends and then do a waitall. We scatter the
   order of sources and destinations among the processes, so that all
   processes don't try to send/recv to/from the same process at the
   same time.

   *** Modification: We post only a small number of isends and irecvs
   at a time and wait on them as suggested by Tony Ladd. ***

   Possible improvements:

   End Algorithm: MPI_Alltoallw
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_intra(const void *sendbuf, const int sendcounts[], const int sdispls[],
                          const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                          const int rdispls[], const MPI_Datatype recvtypes[],
                          MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, i, j;
    int dst, rank;
    int ii, ss, bblock;
    int type_size, recv_extent;
    MPI_Aint true_extent, true_lb;
    MPIR_SCHED_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (sendbuf == MPI_IN_PLACE) {
        int max_size;
        void *tmp_buf = NULL, *adj_tmp_buf = NULL;

        /* The regular MPI_Alltoallw handles MPI_IN_PLACE using pairwise
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
        max_size = 0;
        for (i = 0; i < comm_size; ++i) {
            /* only look at recvtypes/recvcounts because the send vectors are
             * ignored when sendbuf==MPI_IN_PLACE */
            MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
            MPIR_Datatype_get_extent_macro(recvtypes[i], recv_extent);
            max_size = MPL_MAX(max_size, recvcounts[i] * MPL_MAX(recv_extent, true_extent));
        }
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, max_size, mpi_errno, "Ialltoallw tmp_buf");

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

                    MPIR_Type_get_true_extent_impl(recvtypes[i], &true_lb, &true_extent);
                    adj_tmp_buf = (void *)((char*)tmp_buf - true_lb);

                    mpi_errno = MPIR_Sched_send(((char *)recvbuf + rdispls[dst]),
                                                recvcounts[dst], recvtypes[dst], dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Sched_recv(adj_tmp_buf, recvcounts[dst], recvtypes[dst], dst, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    mpi_errno = MPIR_Sched_copy(adj_tmp_buf, recvcounts[dst], recvtypes[dst],
                                                ((char *)recvbuf + rdispls[dst]),
                                                recvcounts[dst], recvtypes[dst], s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
            }
        }
    }
    else {
        bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
        if (bblock == 0) bblock = comm_size;

        /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
        for (ii = 0; ii < comm_size; ii += bblock) {
            ss = comm_size-ii < bblock ? comm_size-ii : bblock;

            /* do the communication -- post ss sends and receives: */
            for (i = 0; i < ss; i++) {
                dst = (rank + i + ii) % comm_size;
                if (recvcounts[dst]) {
                    MPIR_Datatype_get_size_macro(recvtypes[dst], type_size);
                    if (type_size) {
                        mpi_errno = MPIR_Sched_recv((char *)recvbuf+rdispls[dst],
                                                    recvcounts[dst], recvtypes[dst],
                                                    dst, comm_ptr, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
            }

            for (i=0; i<ss; i++) {
                dst = (rank-i-ii+comm_size) % comm_size;
                if (sendcounts[dst]) {
                    MPIR_Datatype_get_size_macro(sendtypes[dst], type_size);
                    if (type_size) {
                        mpi_errno = MPIR_Sched_send((char *)sendbuf+sdispls[dst],
                                                    sendcounts[dst], sendtypes[dst],
                                                    dst, comm_ptr, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
            }

            /* force our block of sends/recvs to complete before starting the next block */
            MPIR_SCHED_BARRIER(s);
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
#define FUNCNAME MPIR_Ialltoallw_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_inter(const void *sendbuf, const int sendcounts[], const int sdispls[],
                          const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                          const int rdispls[], const MPI_Datatype recvtypes[],
                          MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
/* Intercommunicator alltoallw. We use a pairwise exchange algorithm
   similar to the one used in intracommunicator alltoallw. Since the local and
   remote groups can be of different sizes, we first compute the max of
   local_group_size, remote_group_size.  At step i, 0 <= i < max_size, each
   process receives from src = (rank - i + max_size) % max_size if src < remote_size,
   and sends to dst = (rank + i) % max_size if dst < remote_size.

   FIXME: change algorithm to match intracommunicator alltoallw
*/
    int mpi_errno = MPI_SUCCESS;
    int local_size, remote_size, max_size, i;
    int src, dst, rank, sendcount, recvcount;
    char *sendaddr, *recvaddr;
    MPI_Datatype sendtype, recvtype;

    local_size = comm_ptr->local_size;
    remote_size = comm_ptr->remote_size;
    rank = comm_ptr->rank;

    /* Use pairwise exchange algorithm. */
    max_size = MPL_MAX(local_size, remote_size);
    for (i=0; i<max_size; i++) {
        src = (rank - i + max_size) % max_size;
        dst = (rank + i) % max_size;
        if (src >= remote_size) {
            src = MPI_PROC_NULL;
            recvaddr = NULL;
            recvcount = 0;
            recvtype = MPI_DATATYPE_NULL;
        }
        else {
            recvaddr = (char *)recvbuf + rdispls[src];
            recvcount = recvcounts[src];
            recvtype = recvtypes[src];
        }
        if (dst >= remote_size) {
            dst = MPI_PROC_NULL;
            sendaddr = NULL;
            sendcount = 0;
            sendtype = MPI_DATATYPE_NULL;
        }
        else {
            sendaddr = (char *)sendbuf+sdispls[dst];
            sendcount = sendcounts[dst];
            sendtype = sendtypes[dst];
        }

        mpi_errno = MPIR_Sched_send(sendaddr, sendcount, sendtype, dst, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier here */
        mpi_errno = MPIR_Sched_recv(recvaddr, recvcount, recvtype, src, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ialltoallw_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoallw_impl(const void *sendbuf, const int sendcounts[], const int sdispls[],
                         const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                         const int rdispls[], const MPI_Datatype recvtypes[], MPIR_Comm *comm_ptr,
                         MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *reqp = NULL;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    MPIR_Assert(comm_ptr->coll_fns != NULL);
    MPIR_Assert(comm_ptr->coll_fns->Ialltoallw_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Ialltoallw_sched(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, &reqp);
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
#define FUNCNAME MPI_Ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
                   MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLTOALLW);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLTOALLW);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
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
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_ARGNULL(sendcounts,"sendcounts", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sdispls,"sdispls", mpi_errno);
                MPIR_ERRTEST_ARGNULL(sendtypes,"sendtypes", mpi_errno);

                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                        sendcounts == recvcounts &&
                        sendtypes == recvtypes)
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,recvbuf,mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(recvcounts,"recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(rdispls,"rdispls", mpi_errno);
            MPIR_ERRTEST_ARGNULL(recvtypes,"recvtypes", mpi_errno);
            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM && sendbuf == MPI_IN_PLACE) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**sendbuf_inplace");
            }
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLTOALLW);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_ialltoallw", "**mpi_ialltoallw %p %p %p %p %p %p %p %p %C %p", sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
