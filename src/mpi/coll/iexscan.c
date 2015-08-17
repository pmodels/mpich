/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iexscan */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iexscan = PMPI_Iexscan
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iexscan  MPI_Iexscan
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iexscan as PMPI_Iexscan
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm, MPI_Request *request)
                __attribute__((weak,alias("PMPI_Iexscan")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Iexscan
#define MPI_Iexscan PMPI_Iexscan

/* any non-MPI functions go here, especially non-static ones */

/* This is the default implementation of exscan. The algorithm is:

   Algorithm: MPI_Iexscan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

 .vb
   partial_scan = sendbuf;
   mask = 0x1;
   flag = 0;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
            if (rank != 0) {
               if (flag == 0) {
                   recv_buf = tmp_buf;
                   flag = 1;
               }
               else
                   recv_buf = tmp_buf + recvbuf;
            }
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

   End Algorithm: MPI_Exscan
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPID_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size;
    int mask, dst, is_commutative, flag;
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPIR_SCHED_CHKPMEM_DECL(2);

    if (count == 0)
        goto fn_exit;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store partial scan*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(datatype, extent);

    MPIR_SCHED_CHKPMEM_MALLOC(partial_scan, void *, (count*(MPIR_MAX(true_extent,extent))), mpi_errno, "partial_scan");
    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, (count*(MPIR_MAX(true_extent,extent))), mpi_errno, "tmp_buf");
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    mpi_errno = MPID_Sched_copy((sendbuf == MPI_IN_PLACE ? (const void *)recvbuf : sendbuf), count, datatype,
                               partial_scan, count, datatype, s);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    flag = 0;
    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPID_Sched_send(partial_scan, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPID_Sched_recv(tmp_buf, count, datatype, dst, comm_ptr, s);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            MPID_SCHED_BARRIER(s);

            if (rank > dst) {
                mpi_errno = MPID_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                MPID_SCHED_BARRIER(s);

                /* On rank 0, recvbuf is not defined.  For sendbuf==MPI_IN_PLACE
                   recvbuf must not change (per MPI-2.2).
                   On rank 1, recvbuf is to be set equal to the value
                   in sendbuf on rank 0.
                   On others, recvbuf is the scan of values in the
                   sendbufs on lower ranks. */
                if (rank != 0) {
                    if (flag == 0) {
                        /* simply copy data recd from rank 0 into recvbuf */
                        mpi_errno = MPID_Sched_copy(tmp_buf, count, datatype,
                                                    recvbuf, count, datatype, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        MPID_SCHED_BARRIER(s);

                        flag = 1;
                    }
                    else {
                        mpi_errno = MPID_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                        MPID_SCHED_BARRIER(s);
                    }
                }
            }
            else {
                if (is_commutative) {
                    mpi_errno = MPID_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPID_SCHED_BARRIER(s);
                }
                else {
                    mpi_errno = MPID_Sched_reduce(partial_scan, tmp_buf, count, datatype, op, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPID_SCHED_BARRIER(s);

                    mpi_errno = MPID_Sched_copy(tmp_buf, count, datatype,
                                                partial_scan, count, datatype, s);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    MPID_SCHED_BARRIER(s);
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
#define FUNCNAME MPIR_Iexscan_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iexscan_impl(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPID_Comm *comm_ptr, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *reqp = NULL;
    int tag = -1;
    MPID_Sched_t s = MPID_SCHED_NULL;

    *request = MPI_REQUEST_NULL;

    MPIU_Assert(comm_ptr->coll_fns != NULL);
    if (comm_ptr->coll_fns->Iexscan_req != NULL) {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->coll_fns->Iexscan_req(sendbuf, recvbuf, count, datatype, op, comm_ptr, &reqp);
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
    MPIU_Assert(comm_ptr->coll_fns->Iexscan_sched != NULL);
    mpi_errno = comm_ptr->coll_fns->Iexscan_sched(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
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
#define FUNCNAME MPI_Iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Iexscan - Computes the exclusive scan (partial reductions) of data on a
              collection of processes in a nonblocking way


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
int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
                MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IEXSCAN);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_IEXSCAN);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_OP(op, mpi_errno);
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
            MPIR_ERRTEST_COMM_INTRA(comm_ptr, mpi_errno);
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            }

            if (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) {
                MPID_Op *op_ptr = NULL;
                MPID_Op_get_ptr(op, op_ptr);
                MPID_Op_valid_ptr(op_ptr, mpi_errno);
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

    mpi_errno = MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, request);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_IEXSCAN);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpi_iexscan", "**mpi_iexscan %p %p %d %D %O %C %p", sendbuf, recvbuf, count, datatype, op, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
    goto fn_exit;
}
