/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Binomial bcast
 *
 * For short messages, we use a binomial tree algorithm.
 * Cost = lgp.alpha + n.lgp.beta
 */
int MPIR_Bcast_intra_binomial(void *buffer,
                              MPI_Aint count,
                              MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr, int coll_attr)
{
    int rank, comm_size, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint nbytes = 0;
    MPI_Status *status_p;
#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    status_p = &status;
    MPI_Aint recvd_size;
#else
    status_p = MPI_STATUS_IGNORE;
#endif
    int is_contig;
    MPI_Aint type_size;
    void *tmp_buf = NULL;
    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    if (HANDLE_IS_BUILTIN(datatype))
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if (!is_contig) {
        MPIR_CHKLMEM_MALLOC(tmp_buf, nbytes);

        /* TODO: Pipeline the packing and communication */
        if (rank == root) {
            mpi_errno =
                MPIR_Localcopy(buffer, count, datatype, tmp_buf, nbytes, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* Use short message algorithm, namely, binomial tree */

    /* Algorithm:
     * This uses a fairly basic recursive subdivision algorithm.
     * The root sends to the process comm_size/2 away; the receiver becomes
     * a root for a subtree and applies the same process.
     *
     * So that the new root can easily identify the size of its
     * subtree, the (subtree) roots are all powers of two (relative
     * to the root) If m = the first power of 2 such that 2^m >= the
     * size of the communicator, then the subtree at root at 2^(m-k)
     * has size 2^k (with special handling for subtrees that aren't
     * a power of two in size).
     *
     * Do subdivision.  There are two phases:
     * 1. Wait for arrival of data.  Because of the power of two nature
     * of the subtree roots, the source of this message is always the
     * process whose relative rank has the least significant 1 bit CLEARED.
     * That is, process 4 (100) receives from process 0, process 7 (111)
     * from process 6 (110), etc.
     * 2. Forward to my subtree
     *
     * Note that the process that is the tree root is handled automatically
     * by this code, since it has no bits set.  */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;
            if (!is_contig)
                mpi_errno = MPIC_Recv(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, src,
                                      MPIR_BCAST_TAG, comm_ptr, status_p);
            else
                mpi_errno = MPIC_Recv(buffer, count, datatype, src,
                                      MPIR_BCAST_TAG, comm_ptr, status_p);
            MPIR_ERR_CHECK(mpi_errno);
#ifdef HAVE_ERROR_CHECKING
            /* check that we received as much as we expected */
            MPIR_Get_count_impl(status_p, MPIR_BYTE_INTERNAL, &recvd_size);
            MPIR_ERR_CHKANDJUMP2(recvd_size != nbytes, mpi_errno, MPI_ERR_OTHER,
                                 "**collective_size_mismatch",
                                 "**collective_size_mismatch %d %d",
                                 (int) recvd_size, (int) nbytes);
#endif
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB up to (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down one.
     *
     * We can easily change to a different algorithm at any power of two
     * by changing the test (mask > 1) to (mask > block_size)
     *
     * One such version would use non-blocking operations for the last 2-4
     * steps (this also bounds the number of MPI_Requests that would
     * be needed).  */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            dst = rank + mask;
            if (dst >= comm_size)
                dst -= comm_size;
            if (!is_contig)
                mpi_errno = MPIC_Send(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, dst,
                                      MPIR_BCAST_TAG, comm_ptr, coll_attr);
            else
                mpi_errno = MPIC_Send(buffer, count, datatype, dst,
                                      MPIR_BCAST_TAG, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
        mask >>= 1;
    }

    if (!is_contig) {
        if (rank != root) {
            mpi_errno =
                MPIR_Localcopy(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, buffer, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);

        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
