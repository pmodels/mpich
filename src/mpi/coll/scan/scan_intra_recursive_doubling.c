/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

#undef FUNCNAME
#define FUNCNAME MPIR_Scan_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Scan_intra_recursive_doubling(const void *sendbuf,
                                       void *recvbuf,
                                       int count,
                                       MPI_Datatype datatype,
                                       MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    MPI_Status status;
    int rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, is_commutative;
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPIR_CHKLMEM_DECL(2);

    if (count == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store partial scan */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_CHKLMEM_MALLOC(partial_scan, void *, count * (MPL_MAX(extent, true_extent)), mpi_errno,
                        "partial_scan", MPL_MEM_BUFFER);

    /* This eventually gets malloc()ed as a temp buffer, not added to
     * any user buffers */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *) ((char *) partial_scan - true_lb);

    /* need to allocate temporary buffer to store incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)), mpi_errno,
                        "tmp_buf", MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* Since this is an inclusive scan, copy local contribution into
     * recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, partial_scan, count, datatype);
    else
        mpi_errno = MPIR_Localcopy(recvbuf, count, datatype, partial_scan, count, datatype);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                      dst, MPIR_SCAN_TAG, tmp_buf,
                                      count, datatype, dst,
                                      MPIR_SCAN_TAG, comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            if (rank > dst) {
                mpi_errno = MPIR_Reduce_local(tmp_buf, partial_scan, count, datatype, op);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            } else {
                if (is_commutative) {
                    mpi_errno = MPIR_Reduce_local(tmp_buf, partial_scan, count, datatype, op);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                } else {
                    mpi_errno = MPIR_Reduce_local(partial_scan, tmp_buf, count, datatype, op);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                               partial_scan, count, datatype);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        mask <<= 1;
    }

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno) {
            mpi_errno = per_thread->op_errno;
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
