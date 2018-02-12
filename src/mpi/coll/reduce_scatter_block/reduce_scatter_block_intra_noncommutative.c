/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


/* This implementation of MPI_Reduce_scatter_block was obtained by taking
   the implementation of MPI_Reduce_scatter from reduce_scatter.c and replacing
   recvcnts[i] with recvcount everywhere. */


#include "mpiimpl.h"

/* FIXME should we be checking the op_errno here? */

/* Algorithm: Noncommutative
 *
 * Restrictions: Only power-of-two and noncommutative
 *
 * Implements the reduce-scatter butterfly algorithm described in J. L. Traff's
 * "An Improved Algorithm for (Non-commutative) Reduce-Scatter with an
 * Application" from EuroPVM/MPI 2005.
 */

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_block_intra_noncommutative
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Reduce_scatter_block_intra_noncommutative(const void *sendbuf,
                                                   void *recvbuf,
                                                   int recvcount,
                                                   MPI_Datatype datatype,
                                                   MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size = comm_ptr->local_size;
    int rank = comm_ptr->rank;
    int pof2;
    int log2_comm_size;
    int i, k;
    int recv_offset, send_offset;
    int block_size, total_count, size;
    MPI_Aint true_extent, true_lb;
    int buf0_was_inout;
    void *tmp_buf0;
    void *tmp_buf1;
    void *result_ptr;
    MPIR_CHKLMEM_DECL(3);

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    pof2 = 1;
    log2_comm_size = 0;
    while (pof2 < comm_size) {
        pof2 <<= 1;
        ++log2_comm_size;
    }

#ifdef HAVE_ERROR_CHECKING
    /* begin error checking */
    MPIR_Assert(pof2 == comm_size);     /* FIXME this version only works for power of 2 procs */
    /* end error checking */
#endif

    /* size of a block (count of datatype per block, NOT bytes per block) */
    block_size = recvcount;
    total_count = block_size * comm_size;

    MPIR_CHKLMEM_MALLOC(tmp_buf0, void *, true_extent * total_count, mpi_errno, "tmp_buf0",
                        MPL_MEM_BUFFER);
    MPIR_CHKLMEM_MALLOC(tmp_buf1, void *, true_extent * total_count, mpi_errno, "tmp_buf1",
                        MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf0 = (void *) ((char *) tmp_buf0 - true_lb);
    tmp_buf1 = (void *) ((char *) tmp_buf1 - true_lb);

    /* Copy our send data to tmp_buf0.  We do this one block at a time and
     * permute the blocks as we go according to the mirror permutation. */
    for (i = 0; i < comm_size; ++i) {
        mpi_errno =
            MPIR_Localcopy((char *) (sendbuf ==
                                     MPI_IN_PLACE ? (const void *) recvbuf : sendbuf) +
                           (i * true_extent * block_size), block_size, datatype,
                           (char *) tmp_buf0 +
                           (MPL_mirror_permutation(i, log2_comm_size) * true_extent * block_size),
                           block_size, datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    buf0_was_inout = 1;

    send_offset = 0;
    recv_offset = 0;
    size = total_count;
    for (k = 0; k < log2_comm_size; ++k) {
        /* use a double-buffering scheme to avoid local copies */
        char *incoming_data = (buf0_was_inout ? tmp_buf1 : tmp_buf0);
        char *outgoing_data = (buf0_was_inout ? tmp_buf0 : tmp_buf1);
        int peer = rank ^ (0x1 << k);
        size /= 2;

        if (rank > peer) {
            /* we have the higher rank: send top half, recv bottom half */
            recv_offset += size;
        } else {
            /* we have the lower rank: recv top half, send bottom half */
            send_offset += size;
        }

        mpi_errno = MPIC_Sendrecv(outgoing_data + send_offset * true_extent,
                                  size, datatype, peer, MPIR_REDUCE_SCATTER_BLOCK_TAG,
                                  incoming_data + recv_offset * true_extent,
                                  size, datatype, peer, MPIR_REDUCE_SCATTER_BLOCK_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        /* always perform the reduction at recv_offset, the data at send_offset
         * is now our peer's responsibility */
        if (rank > peer) {
            /* higher ranked value so need to call op(received_data, my_data) */
            mpi_errno = MPIR_Reduce_local(incoming_data + recv_offset * true_extent,
                                          outgoing_data + recv_offset * true_extent,
                                          size, datatype, op);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        } else {
            /* lower ranked value so need to call op(my_data, received_data) */
            mpi_errno = MPIR_Reduce_local(outgoing_data + recv_offset * true_extent,
                                          incoming_data + recv_offset * true_extent,
                                          size, datatype, op);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            buf0_was_inout = !buf0_was_inout;
        }

        /* the next round of send/recv needs to happen within the block (of size
         * "size") that we just received and reduced */
        send_offset = recv_offset;
    }

    MPIR_Assert(size == recvcount);

    /* copy the reduced data to the recvbuf */
    result_ptr = (char *) (buf0_was_inout ? tmp_buf0 : tmp_buf1) + recv_offset * true_extent;
    mpi_errno = MPIR_Localcopy(result_ptr, size, datatype, recvbuf, size, datatype);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
