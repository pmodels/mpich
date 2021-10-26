/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "recexchalgo.h"
#include "iallreduce_tsp_recursive_exchange_common.h"

static int do_step2_recv_and_reduce(void *recvbuf,
                                    MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                    int tag, MPIR_Comm * comm, MPIR_TSP_sched_t sched,
                                    MPII_Recexchalgo_t * recexch, int phase, int i,
                                    int recvbuf_ready, int nbr_buf_ready, int *vtx_out);
static int do_step2(void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                    MPI_Op op, int tag, MPIR_Comm * comm, MPI_Aint dt_size,
                    MPIR_TSP_sched_t sched, MPII_Recexchalgo_t * recexch,
                    int pre_reduce_id, int do_dtcopy);
static int iallreduce_sched_intra_recexch_zero(MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched);
static int iallreduce_sched_intra_recexch_single_proc(const void *sendbuf, void *recvbuf,
                                                      MPI_Aint count, MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm,
                                                      MPIR_TSP_sched_t sched);

/* Routine to schedule a recursive exchange based allreduce */
int MPIR_TSP_Iallreduce_sched_intra_recexch(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            int per_nbr_buffer, int do_dtcopy, int k,
                                            MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int is_commutative = MPIR_Op_is_commutative(op);
    int nranks = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);

    if (count == 0) {
        mpi_errno = iallreduce_sched_intra_recexch_zero(comm, k, sched);
        goto fn_exit;
    }

    if (nranks == 1) {
        mpi_errno = iallreduce_sched_intra_recexch_single_proc(sendbuf, recvbuf, count,
                                                               datatype, op, comm, sched);
        goto fn_exit;
    }

    MPI_Aint lb, extent, true_extent, dt_size;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    dt_size = count * extent;


    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    int tag;
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);

    MPII_Recexchalgo_t recexch;
    MPII_Recexchalgo_start(rank, nranks, k, &recexch);
    recexch.per_nbr_buffer = per_nbr_buffer;
    recexch.is_commutative = is_commutative;

    /* allocate memory for receive buffers */
    mpi_errno = MPIR_TSP_Iallreduce_recexch_alloc_nbr_bufs(dt_size, &recexch, sched);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_TSP_Iallreduce_recexch_check_commutative(&recexch, rank);
    MPIR_ERR_CHECK(mpi_errno);

    int dtcopy_id = -1;
    if (recexch.step1_sendto == -1 && !is_inplace) {
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL, &dtcopy_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Step 1 - non-participating ranks send data to participating ranks */
    mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch_step1(sendbuf, recvbuf, count,
                                                              datatype, op, comm,
                                                              tag, dt_size, dtcopy_id,
                                                              &recexch, sched);

    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Step 2 - recursive exchange on power-of-k processes */
    if (recexch.in_step2) {
        mpi_errno = do_step2(recvbuf, count, datatype, op, tag, comm, dt_size,
                             sched, &recexch, dtcopy_id, do_dtcopy);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Step 3: participating ranks send reduced data to non-participating ranks */
    mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch_step3(recvbuf, count, datatype,
                                                              comm, tag, &recexch, sched);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_Recexchalgo_finish(&recexch);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* A single step in step2 -- receive from one of the neighbor into nbr_buf, then
 * reduce into recvbuf. recvbuf_ready and nbr_buf_ready are vertex dependency that
 * indicate the dependency on recvbuf and nbr_buf correspondingly.
 */
static int do_step2_recv_and_reduce(void *recvbuf,
                                    MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                    int tag, MPIR_Comm * comm, MPIR_TSP_sched_t sched,
                                    MPII_Recexchalgo_t * recexch, int phase, int i,
                                    int recvbuf_ready, int nbr_buf_ready, int *vtx_out)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative = recexch->is_commutative;
    int nbr = recexch->step2_nbrs[phase][i];
    void *nbr_buf = MPIR_TSP_Iallreduce_recexch_get_nbr_buf(recexch, phase, i);

    /* -- recv from nbr -- */
    int recv_id;
    mpi_errno = MPIR_TSP_sched_irecv(nbr_buf, count, datatype,
                                     nbr, tag, comm, sched, 1, &nbr_buf_ready, &recv_id);
    MPIR_ERR_CHECK(mpi_errno);
    nbr_buf_ready = recv_id;

    /* -- reduce_local -- */
    int vtcs[2];
    vtcs[0] = nbr_buf_ready;
    vtcs[1] = recvbuf_ready;
    if (is_commutative) {
        mpi_errno = MPIR_TSP_sched_reduce_local(nbr_buf, recvbuf, count, datatype, op,
                                                sched, 2, vtcs, vtx_out);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        if (i < recexch->myidxes[phase]) {
            /* nbr_buf + recvbuf */
            mpi_errno = MPIR_TSP_sched_reduce_local(nbr_buf, recvbuf, count, datatype, op,
                                                    sched, 2, vtcs, vtx_out);
        } else {
            /* recvbuf + nbr_buf */
            int temp_id;
            /* TODO: instead of reduce and copy, can we just amend reduce_local? */
            mpi_errno = MPIR_TSP_sched_reduce_local(recvbuf, nbr_buf, count, datatype, op,
                                                    sched, 2, vtcs, &temp_id);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_TSP_sched_localcopy(nbr_buf, count, datatype,
                                                 recvbuf, count, datatype, sched,
                                                 1, &temp_id, vtx_out);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int do_step2(void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                    MPI_Op op, int tag, MPIR_Comm * comm, MPI_Aint dt_size,
                    MPIR_TSP_sched_t sched, MPII_Recexchalgo_t * recexch,
                    int pre_reduce_id, int do_dtcopy)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int per_nbr_buffer = recexch->per_nbr_buffer;
    int is_commutative = recexch->is_commutative;
    int k = recexch->k;
    int nphases = recexch->step2_nphases;

    int *reduce_ids = MPL_malloc((k - 1) * sizeof(int), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!reduce_ids, mpi_errno, MPI_ERR_OTHER, "**nomem");

    void *tmp_buf;
    if (do_dtcopy) {
        tmp_buf = MPIR_TSP_sched_malloc(dt_size, sched);
        MPIR_ERR_CHKANDJUMP(!tmp_buf, mpi_errno, MPI_ERR_OTHER, "**nomem");
    } else {
        /* directly send from recvbuf */
        tmp_buf = recvbuf;
    }

    int recvbuf_ready = pre_reduce_id;
    int tmp_buf_ready = -1;     /* only used if do_dtcopy */
    int nbr_buf_ready = -1;

    for (int phase = 0; phase < nphases; phase++) {
        int imcast_depend;
        /* -- dtcopy -- */
        if (do_dtcopy) {
            int vtcs[2];
            vtcs[0] = recvbuf_ready;
            vtcs[1] = tmp_buf_ready;
            int dtcopy_id;
            mpi_errno = MPIR_TSP_sched_localcopy(recvbuf, count, datatype,
                                                 tmp_buf, count, datatype, sched,
                                                 2, vtcs, &dtcopy_id);
            MPIR_ERR_CHECK(mpi_errno);
            recvbuf_ready = dtcopy_id;
            tmp_buf_ready = dtcopy_id;
            imcast_depend = tmp_buf_ready;
        } else {
            imcast_depend = recvbuf_ready;
        }
        /* -- send data to all the neighbors -- */
        int imcast_id;
        mpi_errno = MPIR_TSP_sched_imcast(tmp_buf, count, datatype,
                                          recexch->step2_nbrs[phase], k - 1, tag, comm,
                                          sched, 1, &imcast_depend, &imcast_id);
        MPIR_ERR_CHECK(mpi_errno);

        if (do_dtcopy) {
            tmp_buf_ready = imcast_id;
        } else {
            recvbuf_ready = imcast_id;
        }

        /* -- recv data and reduce_local -- */
        for (int i = 0; i < k - 1; i++) {
            mpi_errno = do_step2_recv_and_reduce(recvbuf, count, datatype, op, tag, comm,
                                                 sched, recexch, phase, i,
                                                 recvbuf_ready, nbr_buf_ready, &reduce_ids[i]);
            MPIR_ERR_CHECK(mpi_errno);
            if (!per_nbr_buffer) {
                nbr_buf_ready = reduce_ids[i];
            }
            if (!is_commutative) {
                recvbuf_ready = reduce_ids[i];
            }
        }

        if (is_commutative) {
            /* get a sink dependency on all reduces for this phase */
            int sink_id;
            mpi_errno = MPIR_TSP_sched_selective_sink(sched, k - 1, reduce_ids, &sink_id);
            MPIR_ERR_CHECK(mpi_errno);
            recvbuf_ready = sink_id;
        }
    }

  fn_exit:
    MPL_free(reduce_ids);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int iallreduce_sched_intra_recexch_zero(MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    int nranks = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    int tag;
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);

    MPII_Recexchalgo_t recexch;
    MPII_Recexchalgo_start(rank, nranks, k, &recexch);

    /* Step 1 - non-participating ranks send data to participating ranks */
    mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch_step1_zero(comm, tag, &recexch, sched);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Step 2 */
    if (recexch.in_step2) {
        int imcast_id = -1;
        int *recv_id = MPL_malloc(recexch.step2_nphases * (k - 1) * sizeof(int), MPL_MEM_OTHER);

        for (int phase = 0; phase < recexch.step2_nphases; phase++) {
            /* depend on all previous recv */
            mpi_errno = MPIR_TSP_sched_imcast(NULL, 0, MPI_DATATYPE_NULL,
                                              recexch.step2_nbrs[phase], k - 1,
                                              tag, comm, sched,
                                              phase * (k - 1), recv_id, &imcast_id);
            MPIR_ERR_CHECK(mpi_errno);

            for (int i = 0; i < k - 1; i++) {
                mpi_errno = MPIR_TSP_sched_irecv(NULL, 0, MPI_DATATYPE_NULL,
                                                 recexch.step2_nbrs[phase][i],
                                                 tag, comm, sched, 0, NULL, &recv_id[i]);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
        MPL_free(recv_id);
    }

    mpi_errno = MPIR_TSP_sched_fence(sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    mpi_errno = MPIR_TSP_Iallreduce_sched_intra_recexch_step3_zero(comm, tag, &recexch, sched);
    MPIR_ERR_CHECK(mpi_errno);

    MPII_Recexchalgo_finish(&recexch);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int iallreduce_sched_intra_recexch_single_proc(const void *sendbuf, void *recvbuf,
                                                      MPI_Aint count, MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm,
                                                      MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;

    if (sendbuf == MPI_IN_PLACE) {
        MPIR_TSP_sched_fence(sched);
    } else {
        int dummy_id;
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL, &dummy_id);
    }

    return mpi_errno;
}
