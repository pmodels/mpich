/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "recexchalgo.h"

static int MPIR_TSP_Iallgatherv_sched_intra_recexch_data_exchange(int rank, int nranks, int k,
                                                                  int p_of_k, int log_pofk, int T,
                                                                  void *recvbuf,
                                                                  MPI_Datatype recvtype,
                                                                  size_t recv_extent,
                                                                  const MPI_Aint * recvcounts,
                                                                  const MPI_Aint * displs, int tag,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int partner, offset, count;
    int i, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    /* get the partner with whom I should exchange data */
    partner = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);

    if (rank != partner) {
        /* calculate offset and count of the data to be sent to the partner */
        MPII_Recexchalgo_get_count_and_offset(rank, 0, k, nranks, &count, &offset);

        MPI_Aint send_offset = displs[offset] * recv_extent;
        MPI_Aint send_count = 0;
        for (i = 0; i < count; i++)
            send_count += recvcounts[offset + i];
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "data exchange with %d send_offset %d count %d \n", partner,
                         send_offset, send_count));
        /* send my data to partner */
        mpi_errno =
            MPIR_TSP_sched_isend(((char *) recvbuf + send_offset), send_count, recvtype, partner,
                                 tag, comm, sched, 0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        /* calculate offset and count of the data to be received from the partner */
        MPII_Recexchalgo_get_count_and_offset(partner, 0, k, nranks, &count, &offset);
        MPI_Aint recv_offset = displs[offset] * recv_extent;
        MPI_Aint recv_count = 0;
        for (i = 0; i < count; i++)
            recv_count += recvcounts[offset + i];
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "data exchange with %d recv_offset %d count %d \n", partner,
                         recv_offset, recv_count));
        /* recv data from my partner */
        mpi_errno = MPIR_TSP_sched_irecv(((char *) recvbuf + recv_offset), recv_count, recvtype,
                                         partner, tag, comm, sched, 0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


static int MPIR_TSP_Iallgatherv_sched_intra_recexch_step1(int step1_sendto, int *step1_recvfrom,
                                                          int step1_nrecvs, int is_inplace,
                                                          int rank, int tag, const void *sendbuf,
                                                          void *recvbuf, size_t recv_extent,
                                                          const MPI_Aint * recvcounts,
                                                          const MPI_Aint * displs,
                                                          MPI_Datatype recvtype, int n_invtcs,
                                                          int *invtx, MPIR_Comm * comm,
                                                          MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;


    MPIR_FUNC_ENTER;

    if (step1_sendto != -1) {   /* non-participating rank sends the data to a partcipating rank */
        void *buf_to_send;
        MPI_Aint send_offset = displs[rank] * recv_extent;
        if (is_inplace)
            buf_to_send = ((char *) recvbuf + send_offset);
        else
            buf_to_send = (void *) sendbuf;
        mpi_errno =
            MPIR_TSP_sched_isend(buf_to_send, recvcounts[rank], recvtype, step1_sendto, tag, comm,
                                 sched, 0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {
        for (i = 0; i < step1_nrecvs; i++) {    /* participating rank gets the data from non-participating rank */
            MPI_Aint recv_offset = displs[step1_recvfrom[i]] * recv_extent;
            mpi_errno =
                MPIR_TSP_sched_irecv(((char *) recvbuf + recv_offset),
                                     recvcounts[step1_recvfrom[i]], recvtype, step1_recvfrom[i],
                                     tag, comm, sched, n_invtcs, invtx, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


int MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(int step1_sendto, int step2_nphases,
                                                   int **step2_nbrs, int rank, int nranks, int k,
                                                   int p_of_k, int log_pofk, int T, int *nrecvs_,
                                                   int **recv_id_, int tag, void *recvbuf,
                                                   size_t recv_extent, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * displs, MPI_Datatype recvtype,
                                                   int is_dist_halving, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int phase, i, j, count, nbr, offset, rank_for_offset;
    int x, vtx_id;
    int *recv_id = *recv_id_;
    int nrecvs = 0;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    if (is_dist_halving == 1) {
        phase = step2_nphases - 1;
    } else {
        phase = 0;
    }

    for (j = 0; j < step2_nphases && step1_sendto == -1; j++) {
        /* send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            nbr = step2_nbrs[phase][i];
            if (is_dist_halving == 1)
                rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
            else
                rank_for_offset = rank;
            MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks, &count, &offset);
            MPI_Aint send_offset = displs[offset] * recv_extent;
            MPI_Aint send_count = 0;
            for (x = 0; x < count; x++)
                send_count += recvcounts[offset + x];
            mpi_errno = MPIR_TSP_sched_isend(((char *) recvbuf + send_offset), send_count, recvtype,
                                             nbr, tag, comm, sched, nrecvs, recv_id, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "phase %d nbr is %d send offset %d count %d depend on %d \n", phase,
                             nbr, send_offset, count, ((k - 1) * j)));
        }
        /* receive from the neighbors */
        for (i = 0; i < k - 1; i++) {
            nbr = step2_nbrs[phase][i];
            if (is_dist_halving == 1)
                rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(nbr, nranks, k);
            else
                rank_for_offset = nbr;
            MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks, &count, &offset);
            MPI_Aint recv_offset = displs[offset] * recv_extent;
            MPI_Aint recv_count = 0;
            for (x = 0; x < count; x++)
                recv_count += recvcounts[offset + x];
            mpi_errno =
                MPIR_TSP_sched_irecv(((char *) recvbuf + recv_offset), recv_count, recvtype,
                                     nbr, tag, comm, sched, 0, NULL, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            recv_id[j * (k - 1) + i] = vtx_id;
            nrecvs++;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "phase %d recv from %d recv offset %d cur_count %d recv returns id[%d] %d\n",
                             phase, nbr, recv_offset, count, j * (k - 1) + i,
                             recv_id[j * (k - 1) + i]));
        }
        if (is_dist_halving == 1)
            phase--;
        else
            phase++;
    }

    *nrecvs_ = nrecvs;

    MPIR_FUNC_EXIT;

    return mpi_errno;
}


static int MPIR_TSP_Iallgatherv_sched_intra_recexch_step3(int step1_sendto, int *step1_recvfrom,
                                                          int step1_nrecvs, int step2_nphases,
                                                          void *recvbuf,
                                                          const MPI_Aint * recvcounts, int nranks,
                                                          int k, int nrecvs, int *recv_id, int tag,
                                                          MPI_Datatype recvtype, MPIR_Comm * comm,
                                                          MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int total_count = 0, i, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    /* calculate the total count */
    for (i = 0; i < nranks; i++)
        total_count += recvcounts[i];

    if (step1_sendto != -1) {
        mpi_errno =
            MPIR_TSP_sched_irecv(recvbuf, total_count, recvtype, step1_sendto, tag, comm, sched, 0,
                                 NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        mpi_errno = MPIR_TSP_sched_isend(recvbuf, total_count, recvtype, step1_recvfrom[i],
                                         tag, comm, sched, nrecvs, recv_id, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    MPIR_FUNC_EXIT;

    return mpi_errno;
}

/* Routine to schedule a recursive exchange based allgather */
int MPIR_TSP_Iallgatherv_sched_intra_recexch(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             int is_dist_halving, int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_inplace, i;
    int nranks, rank;
    size_t recv_extent;
    MPI_Aint recv_lb, true_extent;
    int step1_sendto = -1, step2_nphases = 0, step1_nrecvs = 0, p_of_k, T;
    int dtcopy_id, n_invtcs = 0, invtx;
    int is_instep2, log_pofk;
    int *step1_recvfrom = NULL;
    int **step2_nbrs = NULL;
    int nrecvs;
    int *recv_id = NULL;
    int tag;

    MPIR_FUNC_ENTER;

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(recvtype, recv_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recv_lb, &true_extent);
    recv_extent = MPL_MAX(recv_extent, true_extent);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* get the neighbors, the function allocates the required memory */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    is_instep2 = (step1_sendto == -1);  /* whether this rank participates in Step 2 */
    log_pofk = step2_nphases;
    recv_id = (int *) MPL_malloc(sizeof(int) * ((step2_nphases * (k - 1)) + 1), MPL_MEM_COLL);

    if (!is_inplace && is_instep2) {
        /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        mpi_errno =
            MPIR_TSP_sched_localcopy(sendbuf, sendcount, sendtype,
                                     (char *) recvbuf + displs[rank] * recv_extent,
                                     recvcounts[rank], recvtype, sched, 0, NULL, &dtcopy_id);
        MPIR_ERR_CHECK(mpi_errno);
        invtx = dtcopy_id;
        n_invtcs = 1;
    } else {
        n_invtcs = 0;
    }

    /* Step 1 */
    MPIR_TSP_Iallgatherv_sched_intra_recexch_step1(step1_sendto, step1_recvfrom, step1_nrecvs,
                                                   is_inplace, rank, tag, sendbuf, recvbuf,
                                                   recv_extent, recvcounts, displs, recvtype,
                                                   n_invtcs, &invtx, comm, sched);
    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* For distance halving algorithm, exchange the data with digit reversed partner
     * so that finally the data is in the correct order. */
    if (is_dist_halving == 1) {
        if (step1_sendto == -1) {
            MPIR_TSP_Iallgatherv_sched_intra_recexch_data_exchange(rank, nranks, k, p_of_k,
                                                                   log_pofk, T, recvbuf, recvtype,
                                                                   recv_extent, recvcounts, displs,
                                                                   tag, comm, sched);
            mpi_errno = MPIR_TSP_sched_fence(sched);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* Step2 */
    MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(step1_sendto, step2_nphases, step2_nbrs, rank,
                                                   nranks, k, p_of_k, log_pofk, T, &nrecvs,
                                                   &recv_id, tag, recvbuf, recv_extent, recvcounts,
                                                   displs, recvtype, is_dist_halving, comm, sched);

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    MPIR_TSP_Iallgatherv_sched_intra_recexch_step3(step1_sendto, step1_recvfrom, step1_nrecvs,
                                                   step2_nphases, recvbuf, recvcounts, nranks, k,
                                                   nrecvs, recv_id, tag, recvtype, comm, sched);

  fn_exit:
    /* free the memory */
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_free(step1_recvfrom);
    MPL_free(recv_id);

    MPIR_FUNC_EXIT;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
