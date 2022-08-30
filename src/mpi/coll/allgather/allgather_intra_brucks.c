/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Algorithm: Bruck's
 *
 * This algorithm is from the IEEE TPDS Nov 97 paper by Jehoshua Bruck
 * et al.  It is a variant of the disemmination algorithm for barrier.
 * It takes ceiling(lg p) steps.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 * where n is total size of data gathered on each process.
 */
int MPIR_Allgather_intra_brucks(const void *sendbuf,
                                MPI_Aint sendcount,
                                MPI_Datatype sendtype,
                                void *recvbuf,
                                MPI_Aint recvcount,
                                MPI_Datatype recvtype,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint recvtype_extent, recvtype_sz;
    int pof2, src, rem;
    void *tmp_buf = NULL;
    int dst;

    MPIR_CHKLMEM_DECL(1);

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        goto fn_exit;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_sz);

    /* allocate a temporary buffer of the same size as recvbuf. */
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, recvcount * comm_size * recvtype_sz, mpi_errno,
                        "tmp_buf", MPL_MEM_BUFFER);

    /* copy local data to the top of tmp_buf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   tmp_buf, recvcount * recvtype_sz, MPI_BYTE);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno = MPIR_Localcopy((char *) recvbuf + rank * recvcount * recvtype_extent,
                                   recvcount, recvtype, tmp_buf, recvcount * recvtype_sz, MPI_BYTE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* do the first \floor(\lg p) steps */

    MPI_Aint curr_cnt;
    curr_cnt = recvcount;
    pof2 = 1;
    while (pof2 <= comm_size / 2) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        mpi_errno = MPIC_Sendrecv(tmp_buf, curr_cnt * recvtype_sz, MPI_BYTE, dst,
                                  MPIR_ALLGATHER_TAG,
                                  ((char *) tmp_buf + curr_cnt * recvtype_sz),
                                  curr_cnt * recvtype_sz, MPI_BYTE,
                                  src, MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        curr_cnt *= 2;
        pof2 *= 2;
    }

    /* if comm_size is not a power of two, one more step is needed */

    rem = comm_size - pof2;
    if (rem) {
        src = (rank + pof2) % comm_size;
        dst = (rank - pof2 + comm_size) % comm_size;

        mpi_errno = MPIC_Sendrecv(tmp_buf, rem * recvcount * recvtype_sz, MPI_BYTE,
                                  dst, MPIR_ALLGATHER_TAG,
                                  ((char *) tmp_buf + curr_cnt * recvtype_sz),
                                  rem * recvcount * recvtype_sz, MPI_BYTE,
                                  src, MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Rotate blocks in tmp_buf down by (rank) blocks and store
     * result in recvbuf. */

    mpi_errno = MPIR_Localcopy(tmp_buf, (comm_size - rank) * recvcount * recvtype_sz, MPI_BYTE,
                               (char *) recvbuf + rank * recvcount * recvtype_extent,
                               (comm_size - rank) * recvcount, recvtype);
    MPIR_ERR_CHECK(mpi_errno);

    if (rank) {
        mpi_errno = MPIR_Localcopy((char *) tmp_buf +
                                   (comm_size - rank) * recvcount * recvtype_sz,
                                   rank * recvcount * recvtype_sz, MPI_BYTE, recvbuf,
                                   rank * recvcount, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
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
