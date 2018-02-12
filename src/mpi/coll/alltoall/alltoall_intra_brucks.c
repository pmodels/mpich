/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* Algorithm: Bruck's Algorithm
 *
 * This algorithm is from the IEEE TPDS Nov 97 paper by Jehoshua Bruck et al.
 *
 * It is a store-and-forward algorithm that
 * takes lgp steps. Because of the extra communication, the bandwidth
 * requirement is (n/2).lgp.beta.
 *
 * Cost = lgp.alpha + (n/2).lgp.beta
 *
 * where n is the total amount of data a process needs to send to all
 * other processes.
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Alltoall_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Alltoall_intra_brucks(const void *sendbuf,
                               int sendcount,
                               MPI_Datatype sendtype,
                               void *recvbuf,
                               int recvcount,
                               MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, i, pof2;
    MPI_Aint sendtype_extent, recvtype_extent;
    MPI_Aint recvtype_true_extent, recvbuf_extent, recvtype_true_lb;
    int mpi_errno = MPI_SUCCESS, src, dst, rank;
    int mpi_errno_ret = MPI_SUCCESS;
    int block, *displs, count;
    MPI_Aint pack_size, position;
    MPI_Datatype newtype = MPI_DATATYPE_NULL;
    void *tmp_buf;
    MPIR_CHKLMEM_DECL(6);

    if (recvcount == 0)
        return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);
#endif /* HAVE_ERROR_CHECKING */

    /* Get extent of send and recv types */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);

    /* allocate temporary buffer */
    MPIR_Pack_size_impl(recvcount * comm_size, recvtype, &pack_size);
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, pack_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

    /* Do Phase 1 of the algorithim. Shift the data blocks on process i
     * upwards by a distance of i blocks. Store the result in recvbuf. */
    mpi_errno = MPIR_Localcopy((char *) sendbuf +
                               rank * sendcount * sendtype_extent,
                               (comm_size - rank) * sendcount, sendtype, recvbuf,
                               (comm_size - rank) * recvcount, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIR_Localcopy(sendbuf, rank * sendcount, sendtype,
                               (char *) recvbuf +
                               (comm_size - rank) * recvcount * recvtype_extent,
                               rank * recvcount, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    /* Input data is now stored in recvbuf with datatype recvtype */

    /* Now do Phase 2, the communication phase. It takes
     * ceiling(lg p) steps. In each step i, each process sends to rank+2^i
     * and receives from rank-2^i, and exchanges all data blocks
     * whose ith bit is 1. */

    /* allocate displacements array for indexed datatype used in
     * communication */

    MPIR_CHKLMEM_MALLOC(displs, int *, comm_size * sizeof(int), mpi_errno, "displs",
                        MPL_MEM_BUFFER);

    pof2 = 1;
    while (pof2 < comm_size) {
        dst = (rank + pof2) % comm_size;
        src = (rank - pof2 + comm_size) % comm_size;

        /* Exchange all data blocks whose ith bit is 1 */
        /* Create an indexed datatype for the purpose */

        count = 0;
        for (block = 1; block < comm_size; block++) {
            if (block & pof2) {
                displs[count] = block * recvcount;
                count++;
            }
        }

        mpi_errno = MPIR_Type_create_indexed_block_impl(count, recvcount,
                                                        displs, recvtype, &newtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Type_commit_impl(&newtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        position = 0;
        mpi_errno = MPIR_Pack_impl(recvbuf, 1, newtype, tmp_buf, pack_size, &position);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIC_Sendrecv(tmp_buf, position, MPI_PACKED, dst,
                                  MPIR_ALLTOALL_TAG, recvbuf, 1, newtype,
                                  src, MPIR_ALLTOALL_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        MPIR_Type_free_impl(&newtype);

        pof2 *= 2;
    }

    /* Rotate blocks in recvbuf upwards by (rank + 1) blocks. Need
     * a temporary buffer of the same size as recvbuf. */

    /* get true extent of recvtype */
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    recvbuf_extent = recvcount * comm_size * (MPL_MAX(recvtype_true_extent, recvtype_extent));
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, recvbuf_extent, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - recvtype_true_lb);

    mpi_errno = MPIR_Localcopy((char *) recvbuf + (rank + 1) * recvcount * recvtype_extent,
                               (comm_size - rank - 1) * recvcount, recvtype, tmp_buf,
                               (comm_size - rank - 1) * recvcount, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }
    mpi_errno = MPIR_Localcopy(recvbuf, (rank + 1) * recvcount, recvtype,
                               (char *) tmp_buf + (comm_size - rank -
                                                   1) * recvcount * recvtype_extent,
                               (rank + 1) * recvcount, recvtype);
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* Blocks are in the reverse order now (comm_size-1 to 0).
     * Reorder them to (0 to comm_size-1) and store them in recvbuf. */

    for (i = 0; i < comm_size; i++) {
        mpi_errno = MPIR_Localcopy((char *) tmp_buf + i * recvcount * recvtype_extent,
                                   recvcount, recvtype,
                                   (char *) recvbuf + (comm_size - i -
                                                       1) * recvcount * recvtype_extent, recvcount,
                                   recvtype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;
  fn_fail:
    if (newtype != MPI_DATATYPE_NULL)
        MPIR_Type_free_impl(&newtype);
    goto fn_exit;
}
