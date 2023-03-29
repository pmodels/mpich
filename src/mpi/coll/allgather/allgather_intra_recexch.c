/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "recexchalgo.h"

/* Routine to schedule a recursive exchange based allgather. The algorithm is based
 * on the recursive doubling algorithm described by Thakur et al, "Optimization of
 * Collective Communication Operations in MPICH", 2005. The recursive doubling
 * algorithm has been extended for any radix k. A variant of the algorithm called
 * as distance halving (selected by setting recexch_type=1) is based on the
 * paper, Sack et al, "Faster topology-aware collective algorithms through
 * non-minimal communication", 2012.
 * */
int MPIR_Allgather_intra_recexch(const void *sendbuf, MPI_Aint sendcount,
                                 MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                 MPI_Datatype recvtype, MPIR_Comm * comm,
                                 int recexch_type, int k, int single_phase_recv,
                                 MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int is_inplace, i, j;
    int nranks, rank;
    size_t recv_extent;
    MPI_Aint recv_lb, true_extent;
    int step1_sendto = -1, step2_nphases = 0, step1_nrecvs = 0, p_of_k, T;
    int partner, offset, count, send_count, recv_count, send_offset, recv_offset, nbr, phase = 0;
    int rank_for_offset, is_instep2 = 1;
    int *step1_recvfrom = NULL, **step2_nbrs = NULL;
    int num_rreq = 0, num_sreq = 0, comm_alloc = 1, total_phases = 0;
    int iter = 0, recv_phase = 0;
    MPIR_Request *rreqs[MAX_RADIX * 2], *sreqs[MAX_RADIX * 2];
    MPIR_Request **recv_reqs = NULL, **send_reqs = NULL;

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recv_lb, &true_extent);
    recv_extent = MPL_MAX(recv_extent, true_extent);

    /* If k value is greater than the maximum radix for which nbrs
     * are stored in comm, generate nbrs here in the function. Otherwise,
     * get the nbrs information stored in the communicator */
    if (k > MAX_RADIX) {
        comm_alloc = 0;
        /* get the neighbors, the function allocates the required memory */
        MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                       &step1_recvfrom, &step1_nrecvs,
                                       &step2_nbrs, &step2_nphases, &p_of_k, &T);

        /* Allocate requests */
        if (step1_sendto == -1) {
            recv_reqs =
                (MPIR_Request **) MPL_malloc((k - 1) * 2 * sizeof(MPIR_Request *), MPL_MEM_BUFFER);
            MPIR_ERR_CHKANDJUMP(!recv_reqs, mpi_errno, MPI_ERR_OTHER, "**nomem");
            send_reqs =
                (MPIR_Request **) MPL_malloc((k - 1) * 2 * sizeof(MPIR_Request *), MPL_MEM_BUFFER);
            MPIR_ERR_CHKANDJUMP(!send_reqs, mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
    } else {
        int index = k - 2;      /* we calculate starting K = 2 */
        /* If this is the first time a recursive exchange algorithm is called with the given k, populate
         * the nbr calculation in the communicator */
        if (comm->coll.nbrs_defined[index] == 0) {
            comm->coll.nbrs_defined[index] = 1;
            comm->coll.k[index] = k;
            comm->coll.step1_sendto[index] = -1;
            comm->coll.step1_nrecvs[index] = 0;
            comm->coll.step2_nphases[index] = 0;
            mpi_errno =
                MPII_Recexchalgo_get_neighbors(comm->rank, comm->local_size, &comm->coll.k[index],
                                               &comm->coll.step1_sendto[index],
                                               &comm->coll.step1_recvfrom[index],
                                               &comm->coll.step1_nrecvs[index],
                                               &comm->coll.step2_nbrs[index],
                                               &comm->coll.step2_nphases[index],
                                               &comm->coll.pofk[index], &T);
        }
        k = comm->coll.k[index];
        p_of_k = comm->coll.pofk[index];
        step1_sendto = comm->coll.step1_sendto[index];
        step1_recvfrom = comm->coll.step1_recvfrom[index];
        step1_nrecvs = comm->coll.step1_nrecvs[index];
        step2_nbrs = comm->coll.step2_nbrs[index];
        step2_nphases = comm->coll.step2_nphases[index];

        recv_reqs = rreqs;
        send_reqs = sreqs;
    }

    is_instep2 = (step1_sendto == -1);  /* whether this rank participates in Step 2 */

    /* This should have been allocated in one of the branches above, but Klocwork doesn't seem to
     * understand that so this is a hint. */
    MPIR_Assert(recv_reqs);

    if (!is_inplace && is_instep2) {
        /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        mpi_errno =
            MPIR_Localcopy(sendbuf, recvcount, recvtype,
                           (char *) recvbuf + rank * recv_extent * recvcount, recvcount, recvtype);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (step1_sendto != -1) {   /* non-participating rank sends the data to a partcipating rank */
        void *buf_to_send;
        send_offset = rank * recv_extent * recvcount;
        if (is_inplace)
            buf_to_send = ((char *) recvbuf + send_offset);
        else
            buf_to_send = (void *) sendbuf;
        mpi_errno =
            MPIC_Send(buf_to_send, recvcount, recvtype, step1_sendto, MPIR_ALLGATHER_TAG, comm,
                      errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
    } else {
        if (step1_nrecvs) {
            num_rreq = 0;
            for (i = 0; i < step1_nrecvs; i++) {        /* participating rank gets the data from non-participating rank */
                recv_offset = step1_recvfrom[i] * recv_extent * recvcount;
                mpi_errno = MPIC_Irecv(((char *) recvbuf + recv_offset), recvcount, recvtype,
                                       step1_recvfrom[i], MPIR_ALLGATHER_TAG, comm,
                                       &recv_reqs[num_rreq++]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            }
            mpi_errno = MPIC_Waitall(num_rreq, recv_reqs, MPI_STATUSES_IGNORE);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    /* For distance halving algorithm, exchange the data with digit reversed partner
     * so that finally the data is in the correct order. */
    if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING) {
        if (step1_sendto == -1) {
            /* get the partner with whom I should exchange data */
            partner = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
            if (rank != partner) {
                /* calculate offset and count of the data to be sent to the partner */
                MPII_Recexchalgo_get_count_and_offset(rank, 0, k, nranks, &count, &offset);
                send_count = count;
                send_offset = offset * recv_extent * recvcount;
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "data exchange with %d send_offset %d count %d",
                                 partner, send_offset, count));
                MPII_Recexchalgo_get_count_and_offset(partner, 0, k, nranks, &count, &offset);
                recv_count = count;
                recv_offset = offset * recv_extent * recvcount;
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "data exchange with %d recv_offset %d count %d",
                                 partner, recv_offset, count));
                mpi_errno =
                    MPIC_Sendrecv(((char *) recvbuf + send_offset), send_count * recvcount,
                                  recvtype, partner, MPIR_ALLGATHER_TAG,
                                  ((char *) recvbuf + recv_offset), recv_count * recvcount,
                                  recvtype, partner, MPIR_ALLGATHER_TAG, comm, MPI_STATUS_IGNORE,
                                  errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            }
        }
    }

    if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING) {
        phase = step2_nphases - 1;
        recv_phase = step2_nphases - 1;
    } else {
        phase = 0;
        recv_phase = 0;
    }

    for (j = 0; j < step2_nphases && step1_sendto == -1; j++) {
        /* post recvs for two phases. If optimization enabled, post for one phase  */
        total_phases = (single_phase_recv) ? 1 : 2;
        num_sreq = 0;
        num_rreq = 0;
        for (iter = 0; iter < total_phases && (j + iter) < step2_nphases; iter++) {
            for (i = 0; i < k - 1; i++) {
                nbr = step2_nbrs[recv_phase][i];
                if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING)
                    rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(nbr, nranks, k);
                else
                    rank_for_offset = nbr;
                MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j + iter, k, nranks, &count,
                                                      &offset);
                recv_offset = offset * recv_extent * recvcount;
                mpi_errno =
                    MPIC_Irecv(((char *) recvbuf + recv_offset), count * recvcount, recvtype, nbr,
                               MPIR_ALLGATHER_TAG, comm, &recv_reqs[num_rreq++]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
            }
            if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING)
                recv_phase--;
            else
                recv_phase++;
        }

        /* send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            nbr = step2_nbrs[phase][i];
            if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING)
                rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
            else
                rank_for_offset = rank;
            MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks, &count, &offset);
            send_offset = offset * recv_extent * recvcount;
            mpi_errno = MPIC_Isend(((char *) recvbuf + send_offset), count * recvcount, recvtype,
                                   nbr, MPIR_ALLGATHER_TAG, comm, &send_reqs[num_sreq++], errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
        }
        /* wait on prev recvs */
        MPIC_Waitall((k - 1), recv_reqs, MPI_STATUSES_IGNORE);

        if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING)
            phase--;
        else
            phase++;

        if (single_phase_recv == false) {
            j++;
            if (j < step2_nphases) {
                /* send data to all the neighbors once more */
                for (i = 0; i < k - 1; i++) {
                    nbr = step2_nbrs[phase][i];
                    if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING)
                        rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
                    else
                        rank_for_offset = rank;
                    MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks, &count,
                                                          &offset);
                    send_offset = offset * recv_extent * recvcount;
                    mpi_errno =
                        MPIC_Isend(((char *) recvbuf + send_offset), count * recvcount,
                                   recvtype, nbr, MPIR_ALLGATHER_TAG, comm,
                                   &send_reqs[num_sreq++], errflag);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
                }
                /* wait on prev recvs */
                mpi_errno = MPIC_Waitall((k - 1), recv_reqs + (k - 1), MPI_STATUSES_IGNORE);
                if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                    MPIR_ERR_POP(mpi_errno);

                if (recexch_type == MPIR_ALLGATHER_RECEXCH_TYPE_DISTANCE_HALVING)
                    phase--;
                else
                    phase++;
            }
        }
        mpi_errno = MPIC_Waitall(num_sreq, send_reqs, MPI_STATUSES_IGNORE);
        if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
            MPIR_ERR_POP(mpi_errno);
    }

    num_rreq = 0;
    if (step1_sendto != -1) {
        mpi_errno =
            MPIC_Recv(recvbuf, recvcount * nranks, recvtype, step1_sendto, MPIR_ALLGATHER_TAG,
                      comm, MPI_STATUS_IGNORE);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        mpi_errno = MPIC_Isend(recvbuf, recvcount * nranks, recvtype, step1_recvfrom[i],
                               MPIR_ALLGATHER_TAG, comm, &recv_reqs[num_rreq++], errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
    }

    mpi_errno = MPIC_Waitall(num_rreq, recv_reqs, MPI_STATUSES_IGNORE);
    if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
        MPIR_ERR_POP(mpi_errno);

    if (!comm_alloc) {
        /* free the memory */
        for (i = 0; i < step2_nphases; i++)
            MPL_free(step2_nbrs[i]);
        MPL_free(step2_nbrs);
        MPL_free(step1_recvfrom);
        if (is_instep2) {
            MPL_free(recv_reqs);
            MPL_free(send_reqs);
        }
    }

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
