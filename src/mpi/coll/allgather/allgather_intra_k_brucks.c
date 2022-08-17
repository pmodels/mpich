/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"

/* Algorithm: Bruck's
 *
 * This algorithm is based from the IEEE TPDS Nov 97 paper by Jehoshua Bruck
 * et al.  It is a variant of the disemmination algorithm for barrier.
 * steps.It modifies the original bruck algorithm to work with any radix
 * k(k >= 2). It takes logk(p) 'phases' to complete and performs (k - 1)
 * sends and recvs per 'phase'.
 * For cases when p is a perfect power of k,
 *       Cost = logk(p) * alpha + (n/p) * (p - 1) * logk(p) * beta
 * where n is the total amount of data a process needs to send to all
 * other processes. The algorithm also works for non power of k cases.
 */

int
MPIR_Allgather_intra_k_brucks(const void *sendbuf, MPI_Aint sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              MPI_Aint recvcount, MPI_Datatype recvtype, MPIR_Comm * comm, int k,
                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j;
    int nphases = 0;
    int src, dst, p_of_k = 0;   /* Largest power of k that is smaller than 'size' */

    int rank = MPIR_Comm_rank(comm);
    int size = MPIR_Comm_size(comm);
    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int max = size - 1;
    MPIR_Request **reqs;
    int num_reqs = 0;

    MPI_Aint sendtype_extent, sendtype_lb;
    MPI_Aint recvtype_extent, recvtype_lb;
    MPI_Aint sendtype_true_extent, recvtype_true_extent;

#ifdef MPL_USE_DBG_LOGGING
    MPI_Aint recvtype_size, sendtype_size;
#endif

    int delta = 1;
    void *tmp_recvbuf = NULL;
    MPIR_CHKLMEM_DECL(2);
    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, (2 * (k - 1) * sizeof(MPIR_Request *)), mpi_errno,
                        "reqs", MPL_MEM_BUFFER);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Allgather_brucks_radix_k algorithm: num_ranks: %d, k: %d",
                                             size, k));

    if (is_inplace) {
        sendcount = recvcount;
        sendtype = recvtype;
    }

    /* get datatype info of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

#ifdef MPL_USE_DBG_LOGGING
    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "send_type_size: %zu, send_type_extent: %zu, send_count: %ld",
                                             sendtype_size, sendtype_extent, sendcount));
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "recv_type_size: %zu, recv_type_extent: %zu, recv_count: %ld",
                                             recvtype_size, recvtype_extent, recvcount));
#endif

    while (max) {
        nphases++;
        max /= k;
    }

    /* Check if size is power of k */
    if (MPL_ipow(k, nphases) == size)
        p_of_k = 1;

    /* All ranks except `rank 0` need a temporary buffer where it recvs the data.
     * This makes the rotation in the final step easier. */
    if (rank == 0)
        tmp_recvbuf = recvbuf;
    else {
        tmp_recvbuf = (int *) MPL_malloc(size * recvcount * recvtype_extent, MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!tmp_recvbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    /* Step1: copy own data from sendbuf to top of recvbuf. */
    if (is_inplace && rank != 0) {
        mpi_errno = MPIR_Localcopy((char *) recvbuf + rank * recvcount * recvtype_extent,
                                   recvcount, recvtype, tmp_recvbuf, recvcount, recvtype);
    } else if (!is_inplace) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype, tmp_recvbuf, recvcount, recvtype);
    }
    if (mpi_errno) {
        MPIR_ERR_POP(mpi_errno);
    }

    /* All following sends/recvs and copies depend on this dtcopy */

    for (i = 0; i < nphases; i++) {
        num_reqs = 0;
        for (j = 1; j < k; j++) {
            if (delta * j >= size)      /* If the first location exceeds comm size, nothing is to be sent. */
                break;

            dst = (int) (size + (rank - delta * j)) % size;
            src = (int) (rank + delta * j) % size;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                                     "Phase%d/%d:j:%d: src:%d, dst:%d",
                                                     i, nphases, j, src, dst));

            /* Amount of data sent in each cycle = k^i, where i = phase_number.
             * if (size != MPL_ipow(k, power_of_k) send less data in the last phase.
             * This might differ for the different values of j in the last phase. */
            MPI_Aint count;
            if ((i == (nphases - 1)) && (!p_of_k)) {
                count = recvcount * delta;
                MPI_Aint left_count = recvcount * (size - delta * j);
                if (j == k - 1)
                    count = left_count;
                else
                    count = MPL_MIN(count, left_count);
            } else {
                count = recvcount * delta;
            }

            /* Receive at the exact location. */
            mpi_errno = MPIC_Irecv((char *) tmp_recvbuf + j * recvcount * delta * recvtype_extent,
                                   count, recvtype, src, MPIR_ALLGATHER_TAG, comm,
                                   &reqs[num_reqs++]);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                                     "Phase#%d:, k:%d Recv at:%p for count:%d", i,
                                                     k, ((char *) tmp_recvbuf +
                                                         j * recvcount * delta * recvtype_extent),
                                                     count));

            /* Send from the start of recv till `count` amount of data. */
            mpi_errno =
                MPIC_Isend(tmp_recvbuf, count, recvtype, dst, MPIR_ALLGATHER_TAG, comm,
                           &reqs[num_reqs++], errflag);
            if (mpi_errno) {
                MPIR_ERR_POP(mpi_errno);
            }

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                                     "Phase#%d:, k:%d Send from:%p for count:%d",
                                                     i, k, tmp_recvbuf, count));

        }
        MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE, errflag);
        delta *= k;
    }

    if (rank != 0) {    /* No shift required for rank 0 */
        mpi_errno =
            MPIR_Localcopy((char *) tmp_recvbuf + (size - rank) * recvcount * recvtype_extent,
                           rank * recvcount, recvtype, (char *) recvbuf, rank * recvcount,
                           recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }

        mpi_errno = MPIR_Localcopy((char *) tmp_recvbuf, (size - rank) * recvcount, recvtype,
                                   (char *) recvbuf + rank * recvcount * recvtype_extent,
                                   (size - rank) * recvcount, recvtype);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

    if (rank != 0)
        MPL_free(tmp_recvbuf);
    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
