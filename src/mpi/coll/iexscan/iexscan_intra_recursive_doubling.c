/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the machine-independent implementation of exscan. The algorithm is:

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
#define FUNCNAME MPIR_Iexscan_sched_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Iexscan_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s)
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

    /* need to allocate temporary buffer to store partial scan */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_SCHED_CHKPMEM_MALLOC(partial_scan, void *, (count * (MPL_MAX(true_extent, extent))),
                              mpi_errno, "partial_scan", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *) ((char *) partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, (count * (MPL_MAX(true_extent, extent))), mpi_errno,
                              "tmp_buf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    mpi_errno =
        MPIR_Sched_copy((sendbuf == MPI_IN_PLACE ? (const void *) recvbuf : sendbuf), count,
                        datatype, partial_scan, count, datatype, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    flag = 0;
    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIR_Sched_send(partial_scan, count, datatype, dst, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            /* sendrecv, no barrier here */
            mpi_errno = MPIR_Sched_recv(tmp_buf, count, datatype, dst, comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);

            if (rank > dst) {
                mpi_errno = MPIR_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);

                /* On rank 0, recvbuf is not defined.  For sendbuf==MPI_IN_PLACE
                 * recvbuf must not change (per MPI-2.2).
                 * On rank 1, recvbuf is to be set equal to the value
                 * in sendbuf on rank 0.
                 * On others, recvbuf is the scan of values in the
                 * sendbufs on lower ranks. */
                if (rank != 0) {
                    if (flag == 0) {
                        /* simply copy data recd from rank 0 into recvbuf */
                        mpi_errno = MPIR_Sched_copy(tmp_buf, count, datatype,
                                                    recvbuf, count, datatype, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        MPIR_SCHED_BARRIER(s);

                        flag = 1;
                    } else {
                        mpi_errno = MPIR_Sched_reduce(tmp_buf, recvbuf, count, datatype, op, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        MPIR_SCHED_BARRIER(s);
                    }
                }
            } else {
                if (is_commutative) {
                    mpi_errno = MPIR_Sched_reduce(tmp_buf, partial_scan, count, datatype, op, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);
                } else {
                    mpi_errno = MPIR_Sched_reduce(partial_scan, tmp_buf, count, datatype, op, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    MPIR_SCHED_BARRIER(s);

                    mpi_errno = MPIR_Sched_copy(tmp_buf, count, datatype,
                                                partial_scan, count, datatype, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
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
