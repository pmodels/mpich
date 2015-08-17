/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
                  int root, MPI_Comm comm, MPI_Request *request)
                  __attribute__((weak,alias("PMPI_Iscatterv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iscatterv
#define MPI_Iscatterv PMPI_Iscatterv

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of scatterv. The algorithm is:

   Algorithm: MPI_Scatterv

   Since the array of sendcounts is valid only on the root, we cannot
   do a tree algorithm without first communicating the sendcounts to
   other processes. Therefore, we simply use a linear algorithm for the
   scatter, which takes (p-1) steps versus lgp steps for the tree
   algorithm. The bandwidth requirement is the same for both algorithms.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements:

   End Algorithm: MPI_Scatterv
*/
/* this routine handles both intracomms and intercomms */
#undef FUNCNAME
#define FUNCNAME MPIR_Iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[],
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;
    MPI_Aint extent;
    int i;

    rank = comm_ptr->rank;

    /* If I'm the root, then scatter */
    if (((comm_ptr->comm_kind == MPID_INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPID_INTERCOMM) && (root == MPI_ROOT)))
    {
        if (comm_ptr->comm_kind == MPID_INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPID_Datatype_get_extent_macro(sendtype, extent);
        /* We need a check to ensure extent will fit in a
         * pointer. That needs extent * (max count) but we can't get
         * that without looping over the input data. This is at least
         * a minimal sanity check. Maybe add a global var since we do
         * loop over sendcount[] in MPI_Scatterv before calling
         * this? */
        MPIU_Ensure_Aint_fits_in_pointer(MPIU_VOID_PTR_CAST_TO_MPI_AINT sendbuf + extent);

        for (i = 0; i < comm_size; i++) {
            if (sendcounts[i]) {
                if ((comm_ptr->comm_kind == MPID_INTRACOMM) && (i == rank)) {
                    if (recvbuf != MPI_IN_PLACE) {
                        mpi_errno = MPID_Sched_copy(((char *)sendbuf+displs[rank]*extent),
                                                    sendcounts[rank], sendtype,
                                                    recvbuf, recvcount, recvtype, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
                else {
                    mpi_errno = MPID_Sched_send(((char *)sendbuf+displs[i]*extent),
                                                sendcounts[i], sendtype, i, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }
    }

    else if (root != MPI_PROC_NULL) {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (recvcount) {
            mpi_errno = MPID_Sched_recv(recvbuf, recvcount, recvtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iscatterv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscatterv_impl(const void *sendbuf, const int sendcounts[], const int displs[],
                        MPI_Datatype sendtype, void *recvbuf, int recvcount,
                        MPI_Datatype recvtype, int root, MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *reqp = NULL;
    int tag = -1;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    if (comm_ptr->coll_fns->Iscatterv_req != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->coll_fns->Iscatterv_req(sendbuf, sendcounts, displs, sendtype,
                                                           recvbuf, recvcount, recvtype,
                                                           root, comm_ptr, &reqp);
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

    MPIU_Assert(comm_ptr->coll_fns->Iscatterv_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iscatterv_sched(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, s);
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
#define FUNCNAME MPI_Iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
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
                  int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ISCATTERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ISCATTERV);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
            if (recvbuf != MPI_IN_PLACE)
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
            MPID_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
            int i, comm_size, rank;

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPID_INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
                rank = comm_ptr->rank;
                comm_size = comm_ptr->local_size;

                if (rank == root) {
                    for (i=0; i<comm_size; i++) {
                        MPIR_ERRTEST_COUNT(sendcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    for (i=0; i<comm_size; i++) {
                        if (sendcounts[i] > 0) {
                            MPIR_ERRTEST_USERBUFFER(sendbuf,sendcounts[i],sendtype,mpi_errno);
                            break;
                        }
                    }
                    for (i=0; i<comm_size; i++) {
                        if (sendcounts[i] > 0) {
                            MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcounts[i], mpi_errno);
                            break;
                        }
                    }
                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcounts[comm_ptr->rank] != 0 && recvcount != 0) {
                        int sendtype_size;
                        MPID_Datatype_get_size_macro(sendtype, sendtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(recvbuf, (char*)sendbuf + displs[comm_ptr->rank]*sendtype_size, mpi_errno);
                    }

                }
                else
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);

                if (recvbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
                }
            }

            if (comm_ptr->comm_kind == MPID_INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
                if (root == MPI_ROOT) {
                    comm_size = comm_ptr->remote_size;
                    for (i=0; i<comm_size; i++) {
                        MPIR_ERRTEST_COUNT(sendcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    for (i=0; i<comm_size; i++) {
                        if (sendcounts[i] > 0) {
                            MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(sendbuf,sendcounts[i],sendtype,mpi_errno);
                            break;
                        }
                    }
                }
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcount,recvtype,mpi_errno);
                }
            }

            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iscatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ISCATTERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iscatterv", "**mpi_iscatterv %p %p %p %D %p %d %D %d %C %p", sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
