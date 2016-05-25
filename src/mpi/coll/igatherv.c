/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Igatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Igatherv = PMPI_Igatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Igatherv  MPI_Igatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Igatherv as PMPI_Igatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request)
                 __attribute__((weak,alias("PMPI_Igatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Igatherv
#define MPI_Igatherv PMPI_Igatherv

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                  MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int comm_size, rank;
    MPI_Aint extent;
    int min_procs;

    rank = comm_ptr->rank;

    /* If rank == root, then I recv lots, otherwise I send */
    if (((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (root == rank)) ||
        ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) && (root == MPI_ROOT)))
    {
        if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
            comm_size = comm_ptr->local_size;
        else
            comm_size = comm_ptr->remote_size;

        MPIR_Datatype_get_extent_macro(recvtype, extent);
        /* each node can make sure it is not going to overflow aint */
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf + displs[rank] * extent);

        for (i = 0; i < comm_size; i++) {
            if (recvcounts[i]) {
                if ((comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) && (i == rank)) {
                    if (sendbuf != MPI_IN_PLACE) {
                        mpi_errno = MPIR_Sched_copy(sendbuf, sendcount, sendtype,
                                                    ((char *)recvbuf+displs[rank]*extent),
                                                    recvcounts[rank], recvtype, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    }
                }
                else {
                    mpi_errno = MPIR_Sched_recv(((char *)recvbuf+displs[i]*extent),
                                                recvcounts[i], recvtype, i, comm_ptr, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }
    }
    else if (root != MPI_PROC_NULL)
    {
        /* non-root nodes, and in the intercomm. case, non-root nodes on remote side */
        if (sendcount) {
            /* we want local size in both the intracomm and intercomm cases
               because the size of the root's group (group A in the standard) is
               irrelevant here. */
            comm_size = comm_ptr->local_size;

            min_procs = MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS;
            if (min_procs == -1)
                min_procs = comm_size + 1; /* Disable ssend */
            else if (min_procs == 0) /* backwards compatibility, use default value */
                MPIR_CVAR_GET_DEFAULT_INT(GATHERV_INTER_SSEND_MIN_PROCS,&min_procs);

            if (comm_size >= min_procs)
                mpi_errno = MPIR_Sched_ssend(sendbuf, sendcount, sendtype, root, comm_ptr, s);
            else
                mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, root, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Igatherv_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Igatherv_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                       int root, MPIR_Comm *comm_ptr, MPI_Request *request)
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
    MPIR_Assert(comm_ptr->coll_fns->Igatherv_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Igatherv_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm_ptr, s);
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
#define FUNCNAME MPI_Igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Igatherv - Gathers into specified locations from all processes in a group
               in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) containing the number of elements that are received from each process (significant only at root)
. displs - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i (significant only at root)
. recvtype - data type of receive buffer elements (significant only at root) (handle)
. root - rank of receiving process (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (significant only at root) (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IGATHERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IGATHERV);

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
            MPIR_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
            int i, rank, comm_size;

            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
                MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                if (sendbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }

                rank = comm_ptr->rank;
                if (rank == root) {
                    comm_size = comm_ptr->local_size;
                    for (i=0; i<comm_size; i++) {
                        MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }

                    for (i=0; i<comm_size; i++) {
                        if (recvcounts[i] > 0) {
                            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[i],recvtype,mpi_errno);
                            break;
                        }
                    }

                    /* catch common aliasing cases */
                    if (sendbuf != MPI_IN_PLACE && sendtype == recvtype && recvcounts[comm_ptr->rank] != 0 && sendcount != 0) {
                        int recvtype_size;
                        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + displs[comm_ptr->rank]*recvtype_size, mpi_errno);
                    }
                }
                else
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            }

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    comm_size = comm_ptr->remote_size;
                    for (i=0; i<comm_size; i++) {
                        MPIR_ERRTEST_COUNT(recvcounts[i], mpi_errno);
                        MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    }
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPIR_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    for (i=0; i<comm_size; i++) {
                        if (recvcounts[i] > 0) {
                            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcounts[i], mpi_errno);
                            MPIR_ERRTEST_USERBUFFER(recvbuf,recvcounts[i],recvtype,mpi_errno);
                            break;
                        }
                    }
                }
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPIR_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                        MPIR_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                        if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcount,sendtype,mpi_errno);
                }
            }
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IGATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_igatherv", "**mpi_igatherv %p %d %D %p %p %p %D %d %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
