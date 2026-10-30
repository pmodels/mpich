/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
 * Recursive-doubling
 *
 * We use a lgp recursive doubling algorithm. The basic algorithm is
 * given below. (You can replace "+" with any other scan operator.)
 * The result is stored in recvbuf.
 *
 * .vb
 *   recvbuf = sendbuf;
 *   partial_scan = sendbuf;
 *   mask = 0x1;
 *   while (mask < size) {
 *       dst = rank^mask;
 *       if (dst < size) {
 *           send partial_scan to dst;
 *           recv from dst into tmp_buf;
 *           if (rank > dst) {
 *               partial_scan = tmp_buf + partial_scan;
 *               recvbuf = tmp_buf + recvbuf;
 *           }
 *           else {
 *               if (op is commutative)
 *                   partial_scan = tmp_buf + partial_scan;
 *               else {
 *                   tmp_buf = partial_scan + tmp_buf;
 *                   partial_scan = tmp_buf;
 *               }
 *           }
 *       }
 *       mask <<= 1;
 *   }
 * .ve
 *
 */

int MPIR_Scan_intra_recursive_doubling(const void *sendbuf,
                                       void *recvbuf,
                                       MPI_Aint count,
                                       MPI_Datatype datatype,
                                       MPI_Op op, MPIR_Comm * comm_ptr, int coll_attr)
{
    MPI_Status status;
    int rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mask, dst, is_commutative;
    void *partial_scan, *tmp_buf;
    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store partial scan */

    MPI_Aint buf_shift, buf_size;
    MPIR_Datatype_get_buffer_span(datatype, count, &buf_shift, &buf_size);
    MPIR_CHKLMEM_MALLOC(partial_scan, buf_size);

    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *) ((char *) partial_scan - buf_shift);

    /* need to allocate temporary buffer to store incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - buf_shift);

    /* Since this is an inclusive scan, copy local contribution into
     * recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, partial_scan, count, datatype);
    else
        mpi_errno = MPIR_Localcopy(recvbuf, count, datatype, partial_scan, count, datatype);
    MPIR_ERR_CHECK(mpi_errno);

    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                      dst, MPIR_SCAN_TAG, tmp_buf,
                                      count, datatype, dst,
                                      MPIR_SCAN_TAG, comm_ptr, &status, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);

            if (rank > dst) {
                mpi_errno = MPIR_Reduce_local(tmp_buf, partial_scan, count, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);
                mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);
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
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
