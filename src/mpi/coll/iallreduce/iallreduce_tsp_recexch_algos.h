/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., IALLREDUCE_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"
#include "recexchalgo.h"

/* Routine to schedule a recursive exchange based allreduce */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallreduce_sched_intra_recexch
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallreduce_sched_intra_recexch(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, int tag,
                                            MPIR_Comm * comm, int per_nbr_buffer, int k,
                                            MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int is_inplace, i, j;
    int dtcopy_id = -1;
    size_t extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int nranks, rank;
    int step1_sendto = -1, step1_nrecvs, *step1_recvfrom;
    int step2_nphases, **step2_nbrs;
    int p_of_k, T;
    int buf = 0;
    int nvtcs, step1_id, *recv_id, *vtcs;
    int myidx, nbr, phase;
    int counter = 0;
    int *send_id, *reduce_id;
    bool in_step2;
    void *tmp_buf;
    void **step1_recvbuf = NULL;
    void **nbr_buffer;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH);

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    tmp_buf = MPIR_TSP_sched_malloc(count * extent, sched);

    /* if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace && count > 0)
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                     NULL);
        return mpi_errno;
    }

    /* get the neighbors, the function allocates the required memory */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    in_step2 = (step1_sendto == -1);    /* whether this rank participates in Step 2 */
    send_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);        /* to store send vertex ids */
    reduce_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);      /* to store reduce vertex ids */
    recv_id = (int *) MPL_malloc(sizeof(int) * step2_nphases * (k - 1), MPL_MEM_COLL);  /* to store receive vertex ids */
    vtcs = MPL_malloc(sizeof(int) * (step2_nphases) * k, MPL_MEM_COLL); /* to store graph dependencies */
    MPIR_Assert(send_id != NULL && reduce_id != NULL && recv_id != NULL && vtcs != NULL);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Beforeinitial dt copy"));
    if (in_step2 && !is_inplace && count > 0) { /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        dtcopy_id = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy"));

    /* Step 1 */
    if (!in_step2) {
        /* non-participating rank sends the data to a participating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        MPIR_TSP_sched_isend(buf_to_send, count, datatype, step1_sendto, tag, comm, sched, 0, NULL);
    } else {    /* Step 2 participating rank */
        step1_recvbuf = (void **) MPL_malloc(sizeof(void *) * step1_nrecvs, MPL_MEM_COLL);
        MPIR_Assert(step1_recvbuf != NULL);
        if (per_nbr_buffer != 1 && step1_nrecvs > 0)
            step1_recvbuf[0] = MPIR_TSP_sched_malloc(count * extent, sched);

        for (i = 0; i < step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            if (per_nbr_buffer == 1)
                step1_recvbuf[i] = MPIR_TSP_sched_malloc(count * extent, sched);
            else
                step1_recvbuf[i] = step1_recvbuf[0];

            /* recv dependencies */
            nvtcs = 0;
            if (i != 0 && per_nbr_buffer == 0 && count != 0) {
                vtcs[nvtcs++] = reduce_id[i - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "step1 recv depend on reduce_id[%d] %d", i - 1,
                                 reduce_id[i - 1]));
            }
            recv_id[i] = MPIR_TSP_sched_irecv(step1_recvbuf[i], count, datatype,
                                              step1_recvfrom[i], tag, comm, sched, nvtcs, vtcs);
            if (count != 0) {   /* Reduce only if data is present */
                /* setup reduce dependencies */
                nvtcs = 1;
                vtcs[0] = recv_id[i];
                if (is_commutative) {
                    if (!is_inplace) {  /* wait for the data to be copied to recvbuf */
                        vtcs[nvtcs++] = dtcopy_id;
                    }
                } else {        /* if not commutative */
                    /* wait for datacopy to complete if i==0 && !is_inplace else wait for previous reduce */
                    if (i == 0 && !is_inplace)
                        vtcs[nvtcs++] = dtcopy_id;
                    else if (i != 0)
                        vtcs[nvtcs++] = reduce_id[i - 1];
                }
                reduce_id[i] = MPIR_TSP_sched_reduce_local(step1_recvbuf[i], recvbuf, count,
                                                           datatype, op, sched, nvtcs, vtcs);
            }
        }
    }

    step1_id = MPIR_TSP_sched_sink(sched);      /* sink for all the tasks up to end of Step 1 */

    /* Step 2 */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Start Step2"));

    /* allocate memory for receive buffers */
    nbr_buffer = (void **) MPL_malloc(sizeof(void *) * step2_nphases * (k - 1), MPL_MEM_COLL);
    MPIR_Assert(nbr_buffer != NULL);
    buf = 0;
    if (step2_nphases > 0)
        nbr_buffer[0] = MPIR_TSP_sched_malloc(count * extent, sched);
    for (i = 0; i < step2_nphases; i++) {
        for (j = 0; j < (k - 1); j++, buf++) {
            if (per_nbr_buffer)
                nbr_buffer[buf] = MPIR_TSP_sched_malloc(count * extent, sched);
            else
                nbr_buffer[buf] = nbr_buffer[0];
        }
    }

    buf = 0;
    for (phase = 0; phase < step2_nphases && step1_sendto == -1; phase++) {
        if (!is_commutative) {
            /* sort the neighbor list so that receives can be posted in order */
            qsort(step2_nbrs[phase], k - 1, sizeof(int), MPII_Algo_compare_int);
        }
        /* copy the data to a temporary buffer so that sends ands recvs
         * can be posted simultaneosly */
        if (count != 0) {
            if (phase == 0) {   /* wait for Step 1 to complete */
                nvtcs = 1;
                vtcs[0] = step1_id;
            } else {
                /* wait for the previous sends to complete */
                MPIR_Localcopy(send_id, k - 1, MPI_INT, vtcs, k - 1, MPI_INT);
                nvtcs = k - 1;
                if (is_commutative && per_nbr_buffer == 1) {
                    /* wait for all the previous reductions to complete */
                    MPIR_Localcopy(reduce_id, k - 1, MPI_INT, vtcs + nvtcs, k - 1, MPI_INT);
                    nvtcs += k - 1;
                } else {        /* if it is not commutative or if there is only one recv buffer */
                    /* just wait for the last reduce to complete as reductions happen in sequential order */
                    vtcs[nvtcs++] = reduce_id[k - 2];
                }
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "dtcopy from recvbuf to tmpbuf. nvtcs %d counter %d",
                             nvtcs, counter));
            dtcopy_id =
                MPIR_TSP_sched_localcopy(recvbuf, count, datatype, tmp_buf, count, datatype, sched,
                                         nvtcs, vtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Step 2: data copy scheduled"));
        }
        /* myidx is the index in the neighbors list such that
         * all neighbors before myidx have ranks less than my rank
         * and neighbors at and after myidx have rank greater than me.
         * This is useful only in the non-commutative case. */
        myidx = 0;
        /* send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            if (count == 0) {
                if (phase == 0) {
                    nvtcs = 1;
                    vtcs[0] = step1_id;
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "count and phase are 0"));
                } else {        /* wait for all the previous receives to have completed */
                    MPIR_Localcopy(recv_id, phase * (k - 1), MPI_INT, vtcs, phase * (k - 1),
                                   MPI_INT);
                    nvtcs = phase * (k - 1);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "count 0. Depend on all previous %d recvs.",
                                     nvtcs));
                }
            } else {
                nvtcs = 1;
                vtcs[0] = dtcopy_id;
            }

            nbr = step2_nbrs[phase][i];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "sending data to %d. I'm dependent on nvtcs %d", nbr,
                             nvtcs));
            send_id[i] =
                MPIR_TSP_sched_isend(tmp_buf, count, datatype, nbr, tag, comm, sched, nvtcs, vtcs);
            if (rank > nbr) {
                myidx = i + 1;
            }
        }

        /* receive and reduce data from the neighbors to the left */
        for (i = myidx - 1, counter = 0; i >= 0; i--, counter++, buf++) {
            nbr = step2_nbrs[phase][i];
            nvtcs = 0;
            if (count != 0 && per_nbr_buffer == 0 && (phase != 0 || counter != 0)) {
                vtcs[nvtcs++] = (counter == 0) ? reduce_id[k - 2] : reduce_id[counter - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "receiving data to %d. I'm dependent on nvtcs %d vtcs %d counter %d",
                                 nbr, nvtcs, vtcs[0], counter));
            }
            recv_id[buf] =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[buf];
                if (counter == 0 || is_commutative) {
                    vtcs[nvtcs++] = dtcopy_id;
                } else {
                    vtcs[nvtcs++] = reduce_id[counter - 1];
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "reducing data with count %d dependent %d nvtcs counter %d",
                                 count, nvtcs, counter));
                reduce_id[counter] =
                    MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                                sched, nvtcs, vtcs);
            }

        }

        /* receive and reduce data from the neighbors to the right */
        for (i = myidx; i < k - 1; i++, counter++, buf++) {
            nbr = step2_nbrs[phase][i];
            nvtcs = 0;
            if (count != 0 && per_nbr_buffer == 0 && (phase != 0 || counter != 0)) {
                vtcs[nvtcs++] = (counter == 0) ? reduce_id[k - 2] : reduce_id[counter - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "receiving data to %d. I'm dependent nvtcs %d vtcs %d counter %d",
                                 nbr, nvtcs, vtcs[0], counter));
            }
            recv_id[buf] =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[buf];
                if (counter == 0 || is_commutative) {
                    vtcs[nvtcs++] = dtcopy_id;
                } else {
                    vtcs[nvtcs++] = reduce_id[counter - 1];
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "reducing data with count %d dependent %d nvtcs coutner %d right neighbor",
                                 count, nvtcs, counter));
                if (is_commutative) {
                    reduce_id[counter] =
                        MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                                    sched, nvtcs, vtcs);
                } else {
                    reduce_id[counter] =
                        MPIR_TSP_sched_reduce_local(recvbuf, nbr_buffer[buf], count, datatype, op,
                                                    sched, nvtcs, vtcs);
                    reduce_id[counter] =
                        MPIR_TSP_sched_localcopy(nbr_buffer[buf], count, datatype, recvbuf, count,
                                                 datatype, sched, 1, &reduce_id[counter]);
                }
            }
        }
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After Step 2"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        MPIR_TSP_sched_irecv(recvbuf, count, datatype, step1_sendto, tag, comm, sched, 0, NULL);
    } else {
        for (i = 0; i < step1_nrecvs; i++) {
            if (count == 0) {
                nvtcs = step2_nphases * (k - 1);
                MPIR_Localcopy(recv_id, step2_nphases * (k - 1), MPI_INT, vtcs,
                               step2_nphases * (k - 1), MPI_INT);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "Count 0. Step 3. Depends on all prev recvs %d",
                                 nvtcs));
            } else if (is_commutative && per_nbr_buffer == 1) {
                /* If commutative, wait for all the prev reduce calls to complete
                 * since they can happen in any order */
                nvtcs = k - 1;
                MPIR_Localcopy(reduce_id, k - 1, MPI_INT, vtcs, k - 1, MPI_INT);
            } else {
                /* If non commutative, wait for the last reduce to complete, as they happen in sequential order */
                nvtcs = 1;
                vtcs[0] = reduce_id[k - 2];
            }
            MPIR_TSP_sched_isend(recvbuf, count, datatype, step1_recvfrom[i], tag, comm, sched,
                                 nvtcs, vtcs);
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Done Step 3"));

    /* free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_free(step1_recvfrom);
    MPL_free(send_id);
    MPL_free(reduce_id);
    MPL_free(recv_id);
    MPL_free(vtcs);
    MPL_free(nbr_buffer);
    if (in_step2)
        MPL_free(step1_recvbuf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH);

    return mpi_errno;
}


/* Non-blocking recexch based ALLREDUCE */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallreduce_intra_recexch
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallreduce_intra_recexch(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                      MPIR_Request ** req, int recexch_type, int k)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_recexch(sendbuf, recvbuf, count, datatype, op, tag, comm,
                                                recexch_type, k, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
