/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Iallreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Iallreduce = PMPIX_Iallreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Iallreduce  MPIX_Iallreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Iallreduce as PMPIX_Iallreduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Iallreduce
#define MPIX_Iallreduce PMPIX_Iallreduce

/* any non-MPI functions go here, especially non-static ones */

/* implements the naive intracomm allreduce, that is, reduce followed by bcast */
#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_naive
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Iallreduce_naive(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank;

    rank = comm_ptr->rank;

    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Ireduce_intra(recvbuf, NULL, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIR_Ireduce_intra(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPID_SCHED_BARRIER(s);

    mpi_errno = MPIR_Ibcast_intra(recvbuf, count, datatype, 0, comm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_homogeneous;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);


    is_homogeneous = TRUE;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = FALSE;
#endif

    MPIU_Assert(is_homogeneous); /* no hetero for the moment */

    /* TODO port the better algorithms from MPIR_Allreduce_intra to augment the
     * naive implementation */
    mpi_errno = MPIR_Iallreduce_naive(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Iallreduce_inter(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
/* Intercommunicator Allreduce.
   We first do an intercommunicator reduce to rank 0 on left group,
   then an intercommunicator reduce to rank 0 on right group, followed
   by local intracommunicator broadcasts in each group.

   We don't do local reduces first and then intercommunicator
   broadcasts because it would require allocation of a temporary buffer.
*/
    int mpi_errno = MPI_SUCCESS;
    int rank, root;
    MPID_Comm *lcomm_ptr = NULL;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

    rank = comm_ptr->rank;

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPIR_Ireduce_inter(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* don't bcast until the reductions have finished */
    mpi_errno = MPID_Sched_barrier(s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        MPIR_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    lcomm_ptr = comm_ptr->local_comm;

    MPIU_Assert(lcomm_ptr->coll_fns && lcomm_ptr->coll_fns->Ibcast);
    mpi_errno = lcomm_ptr->coll_fns->Ibcast(recvbuf, count, datatype, 0, lcomm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Iallreduce_impl(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPID_Request *reqp = NULL;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPID_Sched_next_tag(comm_ptr, &tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Iallreduce != NULL);
    mpi_errno = comm_ptr->coll_fns->Iallreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_Sched_start(&s, comm_ptr, tag, &reqp);
    if (reqp)
        *request = reqp->handle;
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_Iallreduce
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_Iallreduce - XXX description here

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. count - number of elements in send buffer (non-negative integer)
. datatype - data type of elements of send buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Iallreduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_IALLREDUCE);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_IALLREDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
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
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op *op_ptr = NULL;
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr(op_ptr, mpi_errno);
            }
            else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = ( * MPIR_Op_check_dtype_table[op%16 - 1] )(datatype);
            }

            if (comm_ptr->comm_kind == MPID_INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);

            if (sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_IALLREDUCE);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_iallreduce", "**mpix_iallreduce %p %p %d %D %O %C %p", sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
