/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: MPI_Exscan

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
*/
int MPIR_Exscan_intra_recursive_doubling(const void *sendbuf,
                                         void *recvbuf,
                                         int count,
                                         MPI_Datatype datatype,
                                         MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    MPI_Status status;
    int rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, is_commutative, flag;
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPIR_CHKLMEM_DECL(2);

    if (count == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store partial scan */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_CHKLMEM_MALLOC(partial_scan, void *, (count * (MPL_MAX(true_extent, extent))), mpi_errno,
                        "partial_scan", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *) ((char *) partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, (count * (MPL_MAX(true_extent, extent))), mpi_errno,
                        "tmp_buf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    mpi_errno =
        MPIR_Localcopy((sendbuf == MPI_IN_PLACE ? (const void *) recvbuf : sendbuf), count,
                       datatype, partial_scan, count, datatype);
    MPIR_ERR_CHECK(mpi_errno);

    flag = 0;
    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                      dst, MPIR_EXSCAN_TAG, tmp_buf,
                                      count, datatype, dst,
                                      MPIR_EXSCAN_TAG, comm_ptr, &status, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno);

            if (rank > dst) {
                mpi_errno = MPIR_Reduce_local(tmp_buf, partial_scan, count, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);

                /* On rank 0, recvbuf is not defined.  For sendbuf==MPI_IN_PLACE
                 * recvbuf must not change (per MPI-2.2).
                 * On rank 1, recvbuf is to be set equal to the value
                 * in sendbuf on rank 0.
                 * On others, recvbuf is the scan of values in the
                 * sendbufs on lower ranks. */
                if (rank != 0) {
                    if (flag == 0) {
                        /* simply copy data recd from rank 0 into recvbuf */
                        mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                                   recvbuf, count, datatype);
                        MPIR_ERR_CHECK(mpi_errno);

                        flag = 1;
                    } else {
                        mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                }
            } else {
                if (is_commutative) {
                    mpi_errno = MPIR_Reduce_local(tmp_buf, partial_scan, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    mpi_errno = MPIR_Reduce_local(partial_scan, tmp_buf, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);

                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               partial_scan, count, datatype);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
        mask <<= 1;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
