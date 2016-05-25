/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iscan */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iscan = PMPI_Iscan
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iscan  MPI_Iscan
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iscan as PMPI_Iscan
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op,
              MPI_Comm comm, MPI_Request *request)
              __attribute__((weak,alias("PMPI_Iscan")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iscan
#define MPI_Iscan PMPI_Iscan

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of scan. The algorithm is:

   Algorithm: MPI_Scan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

 .vb
   recvbuf = sendbuf;
   partial_scan = sendbuf;
   mask = 0x1;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
            recvbuf = tmp_buf + recvbuf;
         }
         else {
            if (op is commutative)
               partial_scan = tmp_buf + partial_scan;
            else {
               tmp_buf = partial_scan + tmp_buf;
               partial_scan = tmp_buf;
            }
         }
      }
      mask <<= 1;
   }
 .ve

   End Algorithm: MPI_Scan
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Iscan_rec_dbl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscan_rec_dbl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    int is_commutative;
    int mask, dst, rank, comm_size;
    void *partial_scan = NULL;
    void *tmp_buf = NULL;
    MPIR_SCHED_CHKPMEM_DECL(2);

    if (count == 0)
        goto fn_exit;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store partial scan*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_SCHED_CHKPMEM_MALLOC(partial_scan, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "partial_scan");

    /* This eventually gets malloc()ed as a temp buffer, not added to
     * any user buffers */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "tmp_buf");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* Since this is an inclusive scan, copy local contribution into
       recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype,
                                    recvbuf, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype,
                                    partial_scan, count, datatype, s);
    else
        mpi_errno = MPIR_Sched_copy(recvbuf, count, datatype,
                                    partial_scan, count, datatype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIR_Sched_send(partial_scan, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            if (rank > dst) {
                mpi_errno = MPIR_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            else {
                if (is_commutative) {
                    mpi_errno = MPIR_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
                else {
                    mpi_errno = MPIR_Sched_reduce(partial_scan, tmp_buf, count, datatype, op, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    mpi_errno = MPIR_Sched_copy(tmp_buf, count, datatype,
                                                partial_scan, count, datatype, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                }
            }
        }
        mask <<= 1;
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iscan_rec_dbl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscan_SMP(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    MPIR_Comm *node_comm;
    MPIR_Comm *roots_comm;
    MPI_Aint true_extent, true_lb, extent;
    void *tempbuf = NULL;
    void *prefulldata = NULL;
    void *localfulldata = NULL;
    MPIR_SCHED_CHKPMEM_DECL(3);

    /* In order to use the SMP-aware algorithm, the "op" can be
       either commutative or non-commutative, but we require a
       communicator in which all the nodes contain processes with
       consecutive ranks. */

    if (!MPII_Comm_is_node_consecutive(comm_ptr)) {
        /* We can't use the SMP-aware algorithm, use the generic one */
        return MPIR_Iscan_rec_dbl(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    }

    node_comm = comm_ptr->node_comm;
    roots_comm = comm_ptr->node_roots_comm;
    if (node_comm) {
        MPIR_Assert(node_comm->coll_fns && node_comm->coll_fns->Iscan_sched && node_comm->coll_fns->Ibcast_sched);
    }
    if (roots_comm) {
        MPIR_Assert(roots_comm->coll_fns && roots_comm->coll_fns->Iscan_sched);
    }

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_SCHED_CHKPMEM_MALLOC(tempbuf, void *, count*(MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer");
    tempbuf = (void *)((char*)tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_SCHED_CHKPMEM_MALLOC(prefulldata, void *, count*(MPL_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan");
        prefulldata = (void *)((char*)prefulldata - true_lb);

        if (node_comm != NULL) {
            MPIR_SCHED_CHKPMEM_MALLOC(localfulldata, void *, count*(MPL_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan");
            localfulldata = (void *)((char*)localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
       one process, just copy the raw data. */
    if (node_comm != NULL) {
        mpi_errno = node_comm->coll_fns->Iscan_sched(sendbuf, recvbuf, count, datatype, op, node_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }
    else if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Sched_copy(sendbuf, count, datatype,
                                    recvbuf, count, datatype, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }

    /* get result from local node's last processor which
       contains the reduce result of the whole node. Name it as
       localfulldata. For example, localfulldata from node 1 contains
       reduced data of rank 1,2,3. */
    if (roots_comm != NULL && node_comm != NULL) {
        mpi_errno = MPIR_Sched_recv(localfulldata, count, datatype,
                                    (node_comm->local_size - 1), node_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }
    else if (roots_comm == NULL && node_comm != NULL && node_comm->rank == node_comm->local_size - 1) {
        mpi_errno = MPIR_Sched_send(recvbuf, count, datatype, 0, node_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
    }
    else if (roots_comm != NULL) {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example,
       prefulldata on rank 4 contains reduce result of ranks
       1,2,3,4,5,6. it will be sent to rank 7 which is master
       process of node 3. */
    if (roots_comm != NULL) {
        /* FIXME just use roots_comm->rank instead */
        int roots_rank = MPIR_Get_internode_rank(comm_ptr, rank);
        MPIR_Assert(roots_rank == roots_comm->rank);

        mpi_errno = roots_comm->coll_fns->Iscan_sched(localfulldata, prefulldata, count, datatype, op, roots_comm, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        if (roots_rank != roots_comm->local_size-1) {
            mpi_errno = MPIR_Sched_send(prefulldata, count, datatype, (roots_rank + 1), roots_comm, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
        if (roots_rank != 0) {
            mpi_errno = MPIR_Sched_recv(tempbuf, count, datatype, (roots_rank - 1), roots_comm, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
       scan result. for example, to node 3, it will have reduce result
       of rank 1,2,3,4,5,6 in tempbuf.
       then we should broadcast this result in the local node, and
       reduce it with recvbuf to get final result if nessesary. */

    if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
        /* we aren't on "node 0", so our node leader (possibly us) received
         * "prefulldata" from another leader into "tempbuf" */

        if (node_comm != NULL) {
            mpi_errno = node_comm->coll_fns->Ibcast_sched(tempbuf, count, datatype, 0, node_comm, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }

        /* do reduce on tempbuf and recvbuf, finish scan. */
        mpi_errno = MPIR_Sched_reduce(tempbuf, recvbuf, count, datatype, op, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
fn_exit:
    return mpi_errno;
fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Iscan_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPIR_Comm *comm_ptr, MPI_Request *request)
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

    MPIR_Assert(comm_ptr->coll_fns->Iscan_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iscan_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
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
#define FUNCNAME MPI_Iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iscan - Computes the scan (partial reductions) of data on a collection of
            processes in a nonblocking way

Input Parameters:
+ sendbuf - starting address of the send buffer (choice)
. count - number of elements in input buffer (non-negative integer)
. datatype - data type of elements of input buffer (handle)
. op - operation (handle)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - starting address of the receive buffer (choice)
- request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
              MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ISCAN);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_ISCAN);

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
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
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

            MPIR_ERRTEST_ARGNULL(request,"request", mpi_errno);

            if (sendbuf != MPI_IN_PLACE && count != 0)
                MPIR_ERRTEST_ALIAS_COLL(sendbuf, recvbuf, mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, etc) */
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Iscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_ISCAN);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iscan", "**mpi_iscan %p %p %d %D %O %C %p", sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
