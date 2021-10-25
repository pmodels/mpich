/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "recexchalgo.h"
#include "iallreduce_tsp_recursive_exchange_common.h"

static int alloc_nbr_bufs(int n, MPI_Aint dt_size, int per_nbr_buffer,
                          MPII_Recexchalgo_t * recexch,
                          MPIR_TSP_sched_t sched);
static void *get_nbr_buf(MPII_Recexchalgo_t *recexch, int phase, int i, int per_nbr_buffer);
static int prepare_commutative(MPII_Recexchalgo_t *recexch, int is_commutative, int rank);
static int do_step2_recv_and_reduce(void *recvbuf,
                                    MPI_Aint count, MPI_Datatype datatype, MPI_Op op, 
                                    int tag, MPIR_Comm * comm, MPIR_TSP_sched_t sched,
                                    MPII_Recexchalgo_t *recexch, int phase, int i,
                                    int pre_reduce_id, int last_reduce_id, int *vtx_out);
static int do_step2(void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                    MPI_Op op, int tag, MPIR_Comm * comm, MPI_Aint dt_size,
                    MPIR_TSP_sched_t sched, MPII_Recexchalgo_t *recexch,
                    int pre_reduce_id, int do_dtcopy);
static int iallreduce_sched_intra_recexch_zero(MPIR_Comm * comm, int k, MPIR_TSP_sched_t sched);

/* Routine to schedule a recursive exchange based allreduce */
int MPIR_TSP_Iallreduce_sched_intra_recexch(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            int per_nbr_buffer, int do_dtcopy, int k,
                                            MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    if (count == 0) {
        mpi_errno = iallreduce_sched_intra_recexch_zero(comm, k, sched);
        goto fn_exit;
    } 

    int is_inplace = (sendbuf == MPI_IN_PLACE);
    int is_commutative = MPIR_Op_is_commutative(op);
    int nranks = MPIR_Comm_size(comm);
    int rank = MPIR_Comm_rank(comm);

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

    /* allocate memory for receive buffers */
    mpi_errno = alloc_nbr_bufs(recexch.step2_nphases * (k - 1), dt_size,
                               per_nbr_buffer, &recexch, sched);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = prepare_commutative(&recexch, is_commutative, rank);
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

static int prepare_commutative(MPII_Recexchalgo_t *recexch, int is_commutative, int rank)
{
    int mpi_errno = MPI_SUCCESS;
    int k = recexch->k;
    int nphases = recexch->step2_nphases;
    int *myidxes;

    recexch->is_commutative = is_commutative;
    if (is_commutative) {
        goto fn_exit;
    }

    if (recexch->step1_recvfrom) {
        /* assuming step1_recvfrom is ordered. Or we may add a MPL_sort_int_array */
        MPL_reverse_int_array(recexch->step1_recvfrom, recexch->step1_nrecvs);
    }

    if (nphases > 0) {
        myidxes = (int *) MPL_malloc(nphases * sizeof(int), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!myidxes, mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    for (int phase = 0; phase < nphases; phase++) {
        int *nbrs = recexch->step2_nbrs[phase];
        MPL_sort_int_array(nbrs, k - 1);

        /* myidx is the index in the neighbors list such that
        * all neighbors before myidx have ranks less than my rank
        * and neighbors at and after myidx have rank greater than me.
        * This is useful only in the non-commutative case. */
        int myidx = 0;
        for (int i = 0; i < k - 1; i++) {
            if (rank > nbrs[i]) {
                myidx = i + 1;
            }
        }
        myidxes[phase] = myidx;

        /* NOTE: since we are accumulating into recvbuf, in order to
         * preserve operation order, we need follow the reduction sequence:
         *     myidx-1, myidx-2, ..., 0, myidx, myidx+1, ..., k-1
         * Rather than use spearate for-loops, rearrange the list here so 
         * a single for i=0:k-1 loop will work.
         */
        MPL_reverse_int_array(nbrs, myidx);
    }
    recexch->myidxes = myidxes;

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int do_step2_recv_and_reduce(void *recvbuf,
                                    MPI_Aint count, MPI_Datatype datatype, MPI_Op op, 
                                    int tag, MPIR_Comm * comm, MPIR_TSP_sched_t sched,
                                    MPII_Recexchalgo_t *recexch, int phase, int i,
                                    int recvbuf_ready, int nbr_buf_ready, int *vtx_out)
{
    int mpi_errno = MPI_SUCCESS;
    int per_nbr_buffer = recexch->per_nbr_buffer;
    int is_commutative = recexch->is_commutative;
    int nbr = recexch->step2_nbrs[phase][i];
    void *nbr_buf = get_nbr_buf(recexch, phase, i, per_nbr_buffer);

    /* -- recv from nbr -- */
    int recv_id;
    mpi_errno = MPIR_TSP_sched_irecv(nbr_buf, count, datatype,
                                     nbr, tag, comm, sched,
                                     1, &nbr_buf_ready, &recv_id);
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
                    MPIR_TSP_sched_t sched, MPII_Recexchalgo_t *recexch,
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
    int tmp_buf_ready = -1;  /* only used if do_dtcopy */
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
        int last_reduce_id = -1;
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

static int alloc_nbr_bufs(int n, MPI_Aint dt_size, int per_nbr_buffer,
                          MPII_Recexchalgo_t * recexch,
                          MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    void **bufs = NULL;

    bufs = (void **) MPL_malloc(n * sizeof(void *), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!bufs, mpi_errno, MPI_ERR_OTHER, "**nomem");

    if (!per_nbr_buffer) {
        /* single buffer */
        bufs[0] = MPIR_TSP_sched_malloc(dt_size, sched);
        MPIR_ERR_CHKANDJUMP(!bufs[0], mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 1; i < n; i++) {
            bufs[i] = (char *) bufs[0];
        }
    } else {
        /* multiple buffer */
        bufs[0] = MPIR_TSP_sched_malloc(dt_size * n, sched);
        MPIR_ERR_CHKANDJUMP(!bufs[0], mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 1; i < n; i++) {
            bufs[i] = (char *) bufs[0] + dt_size * i;
        }
    }

  fn_exit:
    recexch->per_nbr_buffer = per_nbr_buffer;
    recexch->nbr_bufs = bufs;
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    if (bufs) {
        MPL_free(bufs);
        bufs = NULL;
    }
    goto fn_exit;
}

static void *get_nbr_buf(MPII_Recexchalgo_t *recexch, int phase, int i, int per_nbr_buffer)
{
    if (per_nbr_buffer) {
        int k = recexch->k;
        return recexch->nbr_bufs[phase * (k - 1) + i];
    } else {
        return recexch->nbr_bufs[0];
    }
}
