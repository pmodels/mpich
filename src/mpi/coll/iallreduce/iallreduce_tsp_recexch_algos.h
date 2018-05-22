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
#include "recexchalgo.h"
#include "tsp_namespace_def.h"
#include "recexchalgo.h"

/* Routine to schedule a pipelined recexch based broadcast */
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
    int is_inplace, is_contig, i, j, p;
    int dtcopy_id = -1;
    size_t type_size, extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int nranks;
    int rank;
    int step1_sendto = -1;
    int step2_nphases;
    int step1_nrecvs;
    int *step1_recvfrom;
    int **step2_nbrs;
    int p_of_k, T;
    int buf = 0;
    int nvtcs, sink_id, *recv_id, *vtcs;
    int myidx, nbr, phase;
    int counter = 0;
    int *send_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);   /* to store send ids */
    int *reduce_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL); /* to store recv_reduce ids */
    bool in_step2;
    void *tmp_buf;
    void **step1_recvbuf;
    void **nbr_buffer;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH);


    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    /* if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace && count > 0)
            MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched, 0,
                                     NULL);
        return 0;
    }

    /* COLL_get_neighbors_recexch function allocates memory
     * to these pointers */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Beforeinitial dt copy\n"));
    /* get the neighbors */
    MPII_Recexchalgo_get_neighbors(rank, nranks, &k, &step1_sendto,
                                   &step1_recvfrom, &step1_nrecvs,
                                   &step2_nbrs, &step2_nphases, &p_of_k, &T);
    in_step2 = (step1_sendto == -1);

    tmp_buf = MPIR_TSP_sched_malloc(count * extent, sched);

    if (in_step2 && !is_inplace && count > 0) {
        /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        dtcopy_id = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy\n"));
    /* Step 1 */
    if (!in_step2) {
        /* non-participating rank sends the data to a partcipating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        MPIR_TSP_sched_isend(buf_to_send, count, datatype, step1_sendto, tag, comm, sched, 0, NULL);
    }

    step1_recvbuf = (void **) MPL_malloc(sizeof(void *) * step1_nrecvs, MPL_MEM_COLL);
    if (per_nbr_buffer != 1 && step1_nrecvs > 0)
        step1_recvbuf[0] = MPIR_TSP_sched_malloc(count * extent, sched);
    recv_id = (int *) MPL_malloc(sizeof(int) * step2_nphases * (k - 1), MPL_MEM_COLL);
    for (i = 0; i < step1_nrecvs; i++) {
        /* participating rank gets data from non-partcipating ranks */
        int vtcs[2];
        buf = (per_nbr_buffer == 1) ? i : 0;
        if (per_nbr_buffer == 1)
            step1_recvbuf[buf] = MPIR_TSP_sched_malloc(count * extent, sched);
        /* recv dependencies */
        if (i == 0 || per_nbr_buffer == 1 || count == 0)
            nvtcs = 0;
        else {
            nvtcs = 1;
            vtcs[0] = reduce_id[i - 1];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "step1 recv depend on reduce_id[%d] %d \n", i - 1,
                             reduce_id[i - 1]));
        }
        recv_id[i] = MPIR_TSP_sched_irecv(step1_recvbuf[buf], count, datatype,
                                          step1_recvfrom[i], tag, comm, sched, nvtcs, vtcs);
        if (count != 0) {       /* Reduce only if data is present */
            /* reduce dependencies */
            if (is_commutative) {
                if (!is_inplace) {
                    /* wait for the data to be copied to recvbuf */
                    nvtcs = 2;
                    vtcs[0] = dtcopy_id;
                    vtcs[1] = recv_id[i];
                } else {
                    /* i have no dependency */
                    nvtcs = 1;
                    vtcs[0] = recv_id[i];
                }
            } else {
                /* if not commutative */
                if (i == 0 && is_inplace) {
                    /* if data is inplace, no need to wait */
                    nvtcs = 1;
                    vtcs[0] = recv_id[i];
                } else {
                    /* wait for datacopy to complete if i==0 else wait for previous recv_reduce */
                    nvtcs = 2;
                    if (i == 0)
                        vtcs[0] = dtcopy_id;
                    else
                        vtcs[0] = reduce_id[i - 1];
                    vtcs[1] = recv_id[i];
                }
            }
            reduce_id[i] = MPIR_TSP_sched_reduce_local(step1_recvbuf[buf], recvbuf, count,
                                                       datatype, op, sched, nvtcs, vtcs);
        }
    }
    sink_id = MPIR_TSP_sched_sink(sched);

    /* Step 2 */
    if (step2_nphases > 2 && count == 0)
        vtcs = MPL_malloc(sizeof(int) * (step2_nphases) * k, MPL_MEM_COLL);
    else
        vtcs = MPL_malloc(sizeof(int) * 2 * k, MPL_MEM_COLL);
    /* used for specifying graph dependencies */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After step1"));

    nbr_buffer = (void **) MPL_malloc(sizeof(void *) * step2_nphases * (k - 1), MPL_MEM_COLL);

    if (step2_nphases > 0)
        nbr_buffer[0] = MPIR_TSP_sched_malloc(count * extent, sched);
    for (i = 0; i < step2_nphases; i++) {
        for (j = 0; j < (k - 1); j++, buf++) {
            if (buf == 0)
                /* memory is already allocated for the first child above */
                continue;
            else {
                if (per_nbr_buffer) {
                    nbr_buffer[buf] = MPIR_TSP_sched_malloc(count * extent, sched);
                } else
                    nbr_buffer[buf] = nbr_buffer[0];
            }
        }
    }

    buf = 0;
    for (phase = 0; phase < step2_nphases && step1_sendto == -1; phase++) {
        if (!is_commutative) {
            /* sort the neighbor list so that receives can be posted in order */
            qsort(step2_nbrs[phase], k - 1, sizeof(int), MPII_Algo_intcmpfn);
        }
        /* copy the data to a temporary buffer so that sends ands recvs
         * can be posted simultaneosly */
        if (count != 0) {
            if (phase == 0) {
                /* wait for Step 1 to complete */
                nvtcs = 1;
                vtcs[0] = sink_id;
            } else {
                nvtcs = k - 1;
                for (i = 0; i < k - 1; i++)
                    vtcs[i] = send_id[i];
                if (is_commutative && per_nbr_buffer == 1) {
                    /* wait for all the previous recv_reduce to complete */
                    for (i = 0; i < k - 1; i++)
                        vtcs[i + nvtcs] = reduce_id[i];
                    nvtcs += k - 1;
                } else {        /* if it is not commutative or if there is only one recv buffer */
                    /* just wait for the last recv_reduce to complete as recv_reduce happen in order */
                    vtcs[nvtcs] = reduce_id[k - 2];
                    nvtcs += 1;
                }
                /* wait for the previous sends to complete and the last recv_reduce */
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "dtcopy from recvbuf to tmpbuf. nvtcs %d counter %d \n",
                             nvtcs, counter));
            dtcopy_id =
                MPIR_TSP_sched_localcopy(recvbuf, count, datatype, tmp_buf, count, datatype, sched,
                                         nvtcs, vtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Step 1: data copy scheduled\n"));
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
                    vtcs[0] = sink_id;
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "count and phase are 0 \n"));
                } else {
                    for (p = 0; p < phase; p++)
                        for (j = 0; j < k - 1; j++)
                            vtcs[p * (k - 1) + j] = recv_id[p * (k - 1) + j];
                    nvtcs = phase * (k - 1);
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                    (MPL_DBG_FDEST, "count 0. Depend on all previous %d recvs. \n",
                                     nvtcs));
                }
            } else {
                nvtcs = 1;
                vtcs[0] = dtcopy_id;
            }
            nbr = step2_nbrs[phase][i];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "sending data to %d. I'm dependant on nvtcs %d \n", nbr,
                             nvtcs));
            send_id[i] =
                MPIR_TSP_sched_isend(tmp_buf, count, datatype, nbr, tag, comm, sched, nvtcs, vtcs);
            if (rank > nbr) {
                myidx = i + 1;
            }
        }

        counter = 0;
        /* receive from the neighbors to the left. Need datacopy here */
        for (i = myidx - 1; i >= 0; i--, buf++) {
            nbr = step2_nbrs[phase][i];
            if (per_nbr_buffer == 1 || (phase == 0 && counter == 0) || count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = (counter == 0) ? reduce_id[k - 2] : reduce_id[counter - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "receiving data to %d. I'm dependant on nvtcs %d vtcs %d counter %d \n",
                                 nbr, nvtcs, vtcs[0], counter));
            }
            recv_id[phase * (k - 1) + i] =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[phase * (k - 1) + i];
                if (counter == 0 || is_commutative) {
                    vtcs[1] = dtcopy_id;
                    nvtcs += 1;
                } else {
                    nvtcs += 1;
                    vtcs[1] = reduce_id[counter - 1];
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "reducing data with count %d dependant %d nvtcs counter %d \n",
                                 count, nvtcs, counter));
                reduce_id[counter++] =
                    MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                                sched, nvtcs, vtcs);
            }

        }

        /* receive from the neighbors to the right */
        for (i = myidx; i < k - 1; i++, buf++) {
            nbr = step2_nbrs[phase][i];
            if (per_nbr_buffer == 1 || (phase == 0 && counter == 0) || count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = (counter == 0) ? reduce_id[k - 2] : reduce_id[counter - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "receiving data to %d. I'm dependant nvtcs %d vtcs %d counter %d \n",
                                 nbr, nvtcs, vtcs[0], counter));
            }
            recv_id[phase * (k - 1) + i] =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[phase * (k - 1) + i];
                if (counter == 0 || is_commutative) {
                    vtcs[1] = dtcopy_id;
                    nvtcs += 1;
                } else {
                    vtcs[1] = reduce_id[counter - 1];
                    nvtcs += 1;
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST,
                                 "reducing data with count %d dependant %d nvtcs coutner %d right neighbor \n",
                                 count, nvtcs, counter));
                /* right reduction */
                reduce_id[counter] = MPIR_TSP_sched_reduce_local(recvbuf, nbr_buffer[buf], count,
                                                                 datatype, op, sched, nvtcs, vtcs);
                reduce_id[counter] = MPIR_TSP_sched_localcopy(nbr_buffer[buf], count, datatype,
                                                              recvbuf, count, datatype, sched, 1,
                                                              &reduce_id[counter]);
                counter++;
            }
        }
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After step2\n"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {
        MPIR_TSP_sched_irecv(recvbuf, count, datatype, step1_sendto, tag, comm, sched, 0, NULL);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        if (count == 0) {
            nvtcs = step2_nphases * (k - 1);
            for (p = 0; p < step2_nphases; p++)
                for (j = 0; j < k - 1; j++)
                    vtcs[p * (k - 1) + j] = recv_id[p * (k - 1) + j];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "count 0. Step 3. depends on all prev recvs %d \n",
                             nvtcs));
        } else if (is_commutative && per_nbr_buffer == 1) {
            /* If commutative, wait for all the prev reduce calls to complete
             * since they can happen in any order */
            nvtcs = k - 1;
            for (j = 0; j < k - 1; j++)
                vtcs[j] = reduce_id[j];
        } else {
            /* If non commutative, wait for the last reduce to complete, as they happen in order */
            nvtcs = 1;
            vtcs[0] = reduce_id[k - 2];
        }
        MPIR_TSP_sched_isend(recvbuf, count, datatype, step1_recvfrom[i], tag, comm, sched, nvtcs,
                             vtcs);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After step3\n"));
  fn_exit:
    /* free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        MPL_free(step2_nbrs[i]);
    MPL_free(step2_nbrs);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "freed step2_nbrs\n"));
    MPL_free(step1_recvfrom);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "freed step1_recvfrom\n"));

    MPL_free(send_id);
    MPL_free(reduce_id);
    MPL_free(recv_id);
    MPL_free(vtcs);
    MPL_free(nbr_buffer);
    MPL_free(step1_recvbuf);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_RECEXCH);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
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
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_RECEXCH);


    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_TSP_sched_create(sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    MPIDU_Sched_next_tag(comm, &tag);

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
