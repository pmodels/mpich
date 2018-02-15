/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
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
#define FUNCNAME MPIR_Ialltoall_sched_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ialltoall_sched_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int nbytes, recvtype_size, recvbuf_extent, newtype_size;
    int rank, comm_size;
    void *tmp_buf = NULL;
    MPI_Aint sendtype_extent, recvtype_extent, recvtype_true_lb, recvtype_true_extent;
    int pof2, dst, src;
    int count, block;
    MPI_Datatype newtype;
    int *displs;
    MPIR_CHKLMEM_DECL(1);       /* displs */
    MPIR_SCHED_CHKPMEM_DECL(2); /* tmp_buf (2x) */

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(sendbuf != MPI_IN_PLACE);       /* we do not handle in-place */
#endif

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    /* allocate temporary buffer */
    /* must be same size as entire recvbuf for Phase 3 */
    nbytes = recvtype_size * recvcount * comm_size;
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

    /* Do Phase 1 of the algorithim. Shift the data blocks on process i
     * upwards by a distance of i blocks. Store the result in recvbuf. */
    mpi_errno = MPIR_Sched_copy(((char *) sendbuf + rank * sendcount * sendtype_extent),
                                (comm_size - rank) * sendcount, sendtype,
                                recvbuf, (comm_size - rank) * recvcount, recvtype, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_copy(sendbuf, rank * sendcount, sendtype,
                                ((char *) recvbuf +
                                 (comm_size - rank) * recvcount * recvtype_extent),
                                rank * recvcount, recvtype, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);
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
        MPIR_Datatype_get_size_macro(newtype, newtype_size);

        /* we will usually copy much less than nbytes */
        mpi_errno = MPIR_Sched_copy(recvbuf, 1, newtype, tmp_buf, newtype_size, MPI_BYTE, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        /* now send and recv in parallel */
        mpi_errno = MPIR_Sched_send(tmp_buf, newtype_size, MPI_BYTE, dst, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_recv(recvbuf, 1, newtype, src, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        MPIR_Type_free_impl(&newtype);

        pof2 *= 2;
    }

    /* Phase 3: Rotate blocks in recvbuf upwards by (rank + 1) blocks. Need
     * a temporary buffer of the same size as recvbuf. */

    /* get true extent of recvtype */
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);

    recvbuf_extent = recvcount * comm_size * (MPL_MAX(recvtype_true_extent, recvtype_extent));
    /* not a leak, old tmp_buf value is still tracked by CHKPMEM macros */
    MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, recvbuf_extent, mpi_errno, "tmp_buf",
                              MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - recvtype_true_lb);

    mpi_errno = MPIR_Sched_copy(((char *) recvbuf + (rank + 1) * recvcount * recvtype_extent),
                                (comm_size - rank - 1) * recvcount, recvtype,
                                tmp_buf, (comm_size - rank - 1) * recvcount, recvtype, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIR_Sched_copy(recvbuf, (rank + 1) * recvcount, recvtype,
                                ((char *) tmp_buf +
                                 (comm_size - rank - 1) * recvcount * recvtype_extent),
                                (rank + 1) * recvcount, recvtype, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_SCHED_BARRIER(s);

    /* Blocks are in the reverse order now (comm_size-1 to 0).
     * Reorder them to (0 to comm_size-1) and store them in recvbuf. */

    for (i = 0; i < comm_size; i++) {
        mpi_errno = MPIR_Sched_copy(((char *) tmp_buf + i * recvcount * recvtype_extent),
                                    recvcount, recvtype,
                                    ((char *) recvbuf +
                                     (comm_size - i - 1) * recvcount * recvtype_extent), recvcount,
                                    recvtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
