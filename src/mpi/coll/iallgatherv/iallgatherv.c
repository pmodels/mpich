/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iallgatherv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallgatherv = PMPI_Iallgatherv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallgatherv  MPI_Iallgatherv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallgatherv as PMPI_Iallgatherv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request)
                    __attribute__((weak,alias("PMPI_Iallgatherv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallgatherv
#define MPI_Iallgatherv PMPI_Iallgatherv

/* This is the default implementation of allgatherv. The algorithm is:

   Algorithm: MPI_Allgatherv

   For short messages and non-power-of-two no. of processes, we use
   the algorithm from the Jehoshua Bruck et al IEEE TPDS Nov 97
   paper. It is a variant of the disemmination algorithm for
   barrier. It takes ceiling(lg p) steps.

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is total size of data gathered on each process.

   For short or medium-size messages and power-of-two no. of
   processes, we use the recursive doubling algorithm.

   Cost = lgp.alpha + n.((p-1)/p).beta

   TODO: On TCP, we may want to use recursive doubling instead of the Bruck
   algorithm in all cases because of the pairwise-exchange property of
   recursive doubling (see Benson et al paper in Euro PVM/MPI
   2003).

   For long messages or medium-size messages and non-power-of-two
   no. of processes, we use a ring algorithm. In the first step, each
   process i sends its contribution to process i+1 and receives
   the contribution from process i-1 (with wrap-around). From the
   second step onwards, each process i forwards to process i+1 the
   data it received from process i-1 in the previous step. This takes
   a total of p-1 steps.

   Cost = (p-1).alpha + n.((p-1)/p).beta

   Possible improvements:

   End Algorithm: MPI_Allgatherv
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_intra_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_intra_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[], const int displs[],
                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size, total_count, recvtype_size;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    if ((total_count*recvtype_size < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) &&
        !(comm_size & (comm_size - 1)))
    {
        /* Short or medium size message and power-of-two no. of processes. Use
         * recursive doubling algorithm */
        mpi_errno = MPIR_Iallgatherv_rec_dbl_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else if (total_count*recvtype_size < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        /* Short message and non-power-of-two no. of processes. Use
         * Bruck algorithm (see description above). */
        mpi_errno = MPIR_Iallgatherv_bruck_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* long message or medium-size message and non-power-of-two
         * no. of processes. Use ring algorithm. */
        mpi_errno = MPIR_Iallgatherv_ring_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_inter_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_inter_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[], const int displs[],
                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgatherv_generic_inter_sched(sendbuf, sendcount,
            sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv_sched(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                           void *recvbuf, const int recvcounts[], const int displs[],
                           MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        mpi_errno = MPIR_Iallgatherv_intra_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iallgatherv_inter_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                          void *recvbuf, const int recvcounts[], const int displs[],
                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPI_Request *request)
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

    mpi_errno = MPID_Iallgatherv_sched(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, s);
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
#define FUNCNAME MPI_Iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iallgatherv - Gathers data from all tasks and deliver the combined data
                  to all tasks in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. sendcount - number of elements in send buffer (non-negative integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - non-negative integer array (of length group size) containing the number of elements that are received from each process
. displs - integer array (of length group size). Entry i specifies the displacement relative to recvbuf at which to place the incoming data from process i
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                    const int recvcounts[], const int displs[], MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLGATHERV);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLGATHERV);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
            }
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
            MPIR_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (sendbuf != MPI_IN_PLACE) {
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPIR_Datatype *sendtype_ptr = NULL;
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                }

                /* catch common aliasing cases */
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM &&
                        sendtype == recvtype &&
                        recvcounts[comm_ptr->rank] != 0 &&
                        sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf, (char*)recvbuf + displs[comm_ptr->rank]*recvtype_size, mpi_errno);
                }
            }

            MPIR_ERRTEST_ARGNULL(recvcounts,"recvcounts", mpi_errno);
            MPIR_ERRTEST_ARGNULL(displs,"displs", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *recvtype_ptr = NULL;
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLGATHERV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iallgatherv", "**mpi_iallgatherv %p %d %D %p %p %p %D %C %p", sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
