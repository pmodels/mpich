/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iallreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iallreduce = PMPI_Iallreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iallreduce  MPI_Iallreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iallreduce as PMPI_Iallreduce
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                   MPI_Op op, MPI_Comm comm, MPI_Request *request)
                   __attribute__((weak,alias("PMPI_Iallreduce")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iallreduce
#define MPI_Iallreduce PMPI_Iallreduce

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_intra_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_intra_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, is_homogeneous, pof2, type_size;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    is_homogeneous = TRUE;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = FALSE;
#endif

    if (!is_homogeneous) {
        mpi_errno = MPIR_Iallreduce_naive_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    /* If op is user-defined or count is less than pof2, use
       recursive doubling algorithm. Otherwise do a reduce-scatter
       followed by allgather. (If op is user-defined,
       derived datatypes are allowed and the user could pass basic
       datatypes on one process and derived on another as long as
       the type maps are the same. Breaking up derived
       datatypes to do the reduce-scatter is tricky, therefore
       using recursive doubling in that case.) */

    if ((count*type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||
        (count < pof2))
    {
        /* use recursive doubling */
        mpi_errno = MPIR_Iallreduce_rec_dbl_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* do a reduce-scatter followed by allgather */
        mpi_errno = MPIR_Iallreduce_redscat_allgather_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_inter_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_inter_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
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
    MPIR_Comm *lcomm_ptr = NULL;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM);

    rank = comm_ptr->rank;

    /* first do a reduce from right group to rank 0 in left group,
       then from left group to rank 0 in right group*/
    if (comm_ptr->is_low_group) {
        /* reduce from right group to rank 0*/
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPID_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce to rank 0 of right group */
        root = 0;
        mpi_errno = MPID_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* reduce to rank 0 of left group */
        root = 0;
        mpi_errno = MPID_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        /* no barrier, these reductions can be concurrent */

        /* reduce from right group to rank 0 */
        root = (rank == 0) ? MPI_ROOT : MPI_PROC_NULL;
        mpi_errno = MPID_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* don't bcast until the reductions have finished */
    mpi_errno = MPIR_Sched_barrier(s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm) {
        MPII_Setup_intercomm_localcomm( comm_ptr );
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    lcomm_ptr = comm_ptr->local_comm;

    mpi_errno = MPID_Ibcast_sched(recvbuf, count, datatype, 0, lcomm_ptr, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_SMP_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_SMP_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    MPIR_Comm *nc;
    MPIR_Comm *nrc;

    if (!MPIR_CVAR_ENABLE_SMP_COLLECTIVES || !MPIR_CVAR_ENABLE_SMP_ALLREDUCE)
        MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "SMP collectives are disabled!");
    MPIR_Assert(MPIR_Comm_is_node_aware(comm_ptr));

    nc = comm_ptr->node_comm;
    nrc = comm_ptr->node_roots_comm;

    is_commutative = MPIR_Op_is_commutative(op);

    /* is the op commutative? We do SMP optimizations only if it is. */
    if (!is_commutative) {
        /* use flat fallback */
        mpi_errno = MPIR_Iallreduce_intra_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        goto fn_exit;
    }

    /* on each node, do a reduce to the local root */
    if (nc != NULL) {
        /* take care of the MPI_IN_PLACE case. For reduce,
           MPI_IN_PLACE is specified only on the root;
           for allreduce it is specified on all processes. */
        if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
            /* IN_PLACE and not root of reduce. Data supplied to this
               allreduce is in recvbuf. Pass that as the sendbuf to reduce. */
            mpi_errno = MPID_Ireduce_sched(recvbuf, NULL, count, datatype, op, 0, nc, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        } else {
            mpi_errno = MPID_Ireduce_sched(sendbuf, recvbuf, count, datatype, op, 0, nc, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    } else {
        /* only one process on the node. copy sendbuf to recvbuf */
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype,
                                        recvbuf, count, datatype, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        MPIR_SCHED_BARRIER(s);
    }

    /* now do an IN_PLACE allreduce among the local roots of all nodes */
    if (nrc != NULL) {
        mpi_errno = MPID_Iallreduce_sched(MPI_IN_PLACE, recvbuf, count, datatype, op, nrc, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* now broadcast the result among local processes */
    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPID_Ibcast_sched(recvbuf, count, datatype, 0, nc, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce_sched(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT) {
            mpi_errno = MPIR_Iallreduce_SMP_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        } else {
            mpi_errno = MPIR_Iallreduce_intra_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        }
    } else {
        mpi_errno = MPIR_Iallreduce_inter_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request)
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

    mpi_errno = MPID_Iallreduce_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
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
#define FUNCNAME MPI_Iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iallreduce - Combines values from all processes and distributes the result
                 back to all processes in a nonblocking way

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
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm,
                   MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IALLREDUCE);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IALLREDUCE);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_COUNT(count, mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
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
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPIR_Op *op_ptr = NULL;
                MPIR_Op_get_ptr(op, op_ptr);
                MPIR_Op_valid_ptr(op_ptr, mpi_errno);
            }
            else if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
                mpi_errno = ( * MPIR_OP_HDL_TO_DTYPE_FN(op) )(datatype);
            }
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, count, mpi_errno);

            if (sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_USERBUFFER(sendbuf,count,datatype,mpi_errno);

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM && count != 0 && sendbuf != MPI_IN_PLACE)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IALLREDUCE);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iallreduce", "**mpi_iallreduce %p %p %d %D %O %C %p", sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
