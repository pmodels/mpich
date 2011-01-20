/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Ibcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Ibcast = PMPIX_Ibcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Ibcast  MPIX_Ibcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Ibcast as PMPIX_Ibcast
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Ibcast
#define MPIX_Ibcast PMPIX_Ibcast

/* any non-MPI functions go here, especially non-static ones */

/* Adds operations to the given schedule that correspond to the specified
 * binomial broadcast.  It does _not_ start the schedule.  This permits callers
 * to build up a larger hierarchical broadcast from multiple invocations of this
 * function. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_binomial
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static int MPIR_Ibcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int mask;
    int comm_size, rank;
    int type_size, is_contig, is_homogeneous;
    int nbytes;
    int relative_rank;
    int src, dst;
    void *tmp_buf = NULL;
    MPIU_CHKPMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if (comm_size == 1) {
        /* nothing to add, this is a useless broadcast */
        goto fn_exit;
    }

    MPID_Datatype_is_contig(datatype, &is_contig);

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

    nbytes = type_size * count;

    if (!is_contig || !is_homogeneous)
    {
        MPIU_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf");

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno = MPID_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_PACKED, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Use short message algorithm, namely, binomial tree */

    /* Algorithm:
       This uses a fairly basic recursive subdivision algorithm.
       The root sends to the process comm_size/2 away; the receiver becomes
       a root for a subtree and applies the same process. 

       So that the new root can easily identify the size of its
       subtree, the (subtree) roots are all powers of two (relative
       to the root) If m = the first power of 2 such that 2^m >= the
       size of the communicator, then the subtree at root at 2^(m-k)
       has size 2^k (with special handling for subtrees that aren't
       a power of two in size).

       Do subdivision.  There are two phases:
       1. Wait for arrival of data.  Because of the power of two nature
       of the subtree roots, the source of this message is always the
       process whose relative rank has the least significant 1 bit CLEARED.
       That is, process 4 (100) receives from process 0, process 7 (111)
       from process 6 (110), etc.
       2. Forward to my subtree

       Note that the process that is the tree root is handled automatically
       by this code, since it has no bits set.  */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask; 
            if (src < 0) src += comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPID_Sched_recv(tmp_buf, nbytes, MPI_BYTE, src, comm_ptr, s);
            else
                mpi_errno = MPID_Sched_recv(buffer, count, datatype, src, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
       set from the LSB upto (but not including) mask.  Because of
       the "not including", we start by shifting mask back down one.

       We can easily change to a different algorithm at any power of two
       by changing the test (mask > 1) to (mask > block_size) 

       One such version would use non-blocking operations for the last 2-4
       steps (this also bounds the number of MPI_Requests that would
       be needed).  */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size) dst -= comm_size;
            if (!is_contig || !is_homogeneous)
                mpi_errno = MPID_Sched_send(tmp_buf, nbytes, MPI_BYTE, dst, comm_ptr, s);
            else
                mpi_errno = MPID_Sched_send(buffer, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* NOTE: This is departure from MPIR_Bcast_binomial.  A true analog
             * would put an MPID_Sched_barrier here after every send. */
        }
        mask >>= 1;
    }

    if (!is_contig || !is_homogeneous) {
        if (rank != root) {
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPID_Sched_copy(tmp_buf, nbytes, MPI_PACKED, buffer, count, datatype, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPID_Sched_cb(&MPIR_Sched_cb_free_buf, tmp_buf, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    MPIU_CHKPMEM_COMMIT();
fn_exit:
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_SMP
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibcast_SMP(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int type_size, is_homogeneous;
    int nbytes=0;

#if !defined(USE_SMP_COLLECTIVES)
    MPID_Abort(comm_ptr, MPI_ERR_OTHER, 1, "SMP collectives are disabled!");
#endif
    MPIU_Assert(MPIR_Comm_is_node_aware(comm_ptr));

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    MPIU_Assert(is_homogeneous); /* we don't handle the hetero case yet */
    if (comm_ptr->node_comm) {
        MPIU_Assert(comm_ptr->node_comm->coll_fns);
        MPIU_Assert(comm_ptr->node_comm->coll_fns->Ibcast);
    }
    if (comm_ptr->node_roots_comm) {
        MPIU_Assert(comm_ptr->node_roots_comm->coll_fns);
        MPIU_Assert(comm_ptr->node_roots_comm->coll_fns->Ibcast);
    }

    /* MPI_Type_size() might not give the accurate size of the packed
     * datatype for heterogeneous systems (because of padding, encoding,
     * etc). On the other hand, MPI_Pack_size() can become very
     * expensive, depending on the implementation, especially for
     * heterogeneous systems. We want to use MPI_Type_size() wherever
     * possible, and MPI_Pack_size() in other places.
     */
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

    nbytes = type_size * count;

    /* TODO insert packing here */

    if ((nbytes < MPIR_PARAM_BCAST_SHORT_MSG_SIZE) || (comm_ptr->local_size < MPIR_PARAM_BCAST_MIN_PROCS))
    {
        /* send to intranode-rank 0 on the root's node */
        if (comm_ptr->node_comm != NULL &&
            MPIU_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */ 
        {                                                /* and is on our node (!-1) */
            if (root == comm_ptr->rank) {
                mpi_errno = MPID_Sched_send(buffer, count, datatype, 0, comm_ptr->node_comm, s); 
            }
            else if (0 == comm_ptr->node_comm->rank) {
                mpi_errno = MPID_Sched_recv(buffer, count, datatype, MPIU_Get_intranode_rank(comm_ptr, root),
                                            comm_ptr->node_comm, s);
            }
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        /* perform the internode broadcast */
        if (comm_ptr->node_roots_comm != NULL)
        {
            mpi_errno = comm_ptr->node_roots_comm->coll_fns->Ibcast(buffer, count, datatype,
                                                                    MPIU_Get_internode_rank(comm_ptr, root),
                                                                    comm_ptr->node_roots_comm, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

            /* don't allow the local ops for the intranode phase to start until this has completed */
            mpi_errno = MPID_Sched_barrier(s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        }
        /* perform the intranode broadcast on all except for the root's node */
        if (comm_ptr->node_comm != NULL)
        {
            mpi_errno = comm_ptr->node_comm->coll_fns->Ibcast(buffer, count, datatype, 0, comm_ptr->node_comm, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
    else /* (nbytes > MPIR_PARAM_BCAST_SHORT_MSG_SIZE) && (comm_ptr->size >= MPIR_PARAM_BCAST_MIN_PROCS) */
    {
        /* supposedly...
           smp+doubling good for pof2
           reg+ring better for non-pof2 */
        if (nbytes < MPIR_PARAM_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL))
        {
            /* medium-sized msg and pof2 np */

            /* perform the intranode broadcast on the root's node */
            if (comm_ptr->node_comm != NULL &&
                MPIU_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */ 
            {                                                /* and is on our node (!-1) */
                /* was binomial */
                mpi_errno = comm_ptr->node_comm->coll_fns->Ibcast(buffer, count, datatype,
                                                                  MPIU_Get_intranode_rank(comm_ptr, root),
                                                                  comm_ptr->node_comm, s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                mpi_errno = MPID_Sched_barrier(s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }

            /* FIXME do we need barriers in here at all? */

            /* perform the internode broadcast */
            if (comm_ptr->node_roots_comm != NULL)
            {
#if 0
                if (MPIU_is_pof2(comm_ptr->node_roots_comm->local_size, NULL))
                {
                    /* was scatter doubling allgather */
                }
                else
                {
                    /* was scatter ring allgather */
                }
#endif
                mpi_errno = comm_ptr->node_roots_comm->coll_fns->Ibcast(buffer, count, datatype,
                                                                        MPIU_Get_internode_rank(comm_ptr, root),
                                                                        comm_ptr->node_roots_comm, s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                /* don't allow the local ops for the intranode phase to start until this has completed */
                mpi_errno = MPID_Sched_barrier(s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }

            /* perform the intranode broadcast on all except for the root's node */
            if (comm_ptr->node_comm != NULL &&
                MPIU_Get_intranode_rank(comm_ptr, root) <= 0) /* 0 if root was local root too */
            {                                                 /* -1 if different node than root */
                /* was binomial */
                mpi_errno = comm_ptr->node_comm->coll_fns->Ibcast(buffer, count, datatype, 0, comm_ptr->node_comm, s);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        }
        else /* large msg or non-pof2 */
        {
            /* TODO port scatter_ring_allgather to NBC */
            /*
            mpi_errno = MPIR_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            */
            mpi_errno = MPIR_Ibcast_binomial(buffer, count, datatype, root, comm_ptr, s);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }


fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


/* Provides a generic "flat" broadcast that doesn't know anything about hierarchy.  It will choose
 * between several different algorithms based on the given parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibcast_intra(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTRACOMM);

    /* simplistic implementation for now */
    mpi_errno = MPIR_Ibcast_binomial(buffer, count, datatype, root, comm_ptr, s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Provides a generic "flat" broadcast for intercommunicators that doesn't know
 * anything about hierarchy.  It will choose between several different
 * algorithms based on the given parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibcast_inter(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(comm_ptr->comm_kind == MPID_INTERCOMM);

    /* Intercommunicator broadcast.
     * Root sends to rank 0 in remote group. Remote group does local
     * intracommunicator broadcast. */
    if (root == MPI_PROC_NULL)
    {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    }
    else if (root == MPI_ROOT)
    {
        /* root sends to rank 0 on remote group and returns */
        mpi_errno = MPID_Sched_send(buffer, count, datatype, 0, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else
    {
        /* remote group. rank 0 on remote group receives from root */
        mpi_errno = MPID_Sched_recv(buffer, count, datatype, root, comm_ptr, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (comm_ptr->local_comm == NULL) {
            mpi_errno = MPIR_Setup_intercomm_localcomm(comm_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIR_Ibcast_intra(buffer, count, datatype, root, comm_ptr->local_comm, s);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Ibcast_impl(void *buffer, int count, MPI_Datatype datatype, int root, MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPID_Request *reqp = NULL;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    mpi_errno = MPID_Sched_next_tag(&tag);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPID_Sched_create(&s);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    MPIU_Assert(comm_ptr->coll_fns->Ibcast != NULL);
    mpi_errno = comm_ptr->coll_fns->Ibcast(buffer, count, datatype, root, comm_ptr, s);
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
#define FUNCNAME MPIX_Ibcast
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_Ibcast - XXX description here

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (non-negative integer)
- comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_IBCAST);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_IBCAST);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
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

            MPID_Comm_valid_ptr(comm_ptr, mpi_errno);
            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, request);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_IBCAST);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_ibcast", "**mpix_ibcast %p %d %D %C %p", buffer, count, datatype, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
