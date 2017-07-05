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

#include "coll_recexch_util.h"

#ifndef COLL_NAMESPACE
#error "The tree template must be namespaced with COLL_NAMESPACE"
#endif


/*This function calculates the ranks to/from which the
data is sent/recvd in various steps/phases of recursive koupling
algorithm. Recursive Koupling is divided into three steps - Step 1, 2 and 3.
Step 2 is the main step that does the recursive koupling.
Step 1 and Step 3 are done to handle the case of non-power-of-k number of ranks.
Only p_of_k (largest power of k that is less than n) ranks participate in Step 2.
In Step 1, the non partcipating ranks send their data to ranks
participating in Step 2. In Step 3, the ranks participating in Step 2 send
the final data to non-partcipating ranks.

Step 2 is further divided into log_k(p_of_k) phases.

Ideally, this function belongs to recexch_util.h file but because this
function uses TSP_allocate_mem, it had to be put here
*/
MPL_STATIC_INLINE_PREFIX
    int COLL_get_neighbors_recexch(int rank, int nranks, int *k_, int *step1_sendto,
                                   int **step1_recvfrom_, int *step1_nrecvs,
                                   int ***step2_nbrs_, int *step2_nphases, int *p_of_k_, int *T_)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_GET_NEIGHBORS_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_GET_NEIGHBORS_RECEXCH);

    int i, j, k;
    k = *k_;
    if (nranks < k)     /*If size of the communicator is less than k, reduce the value of k */
        k = (nranks > 2) ? nranks : 2;
    *k_ = k;
    /*p_of_k is the largest power of k that is less than nranks */
    int p_of_k = 1, log_p_of_k = 0, rem, T, newrank;

    while (p_of_k <= nranks) {
        p_of_k *= k;
        log_p_of_k++;
    }

    p_of_k /= k;
    log_p_of_k--;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"allocate memory for storing communication pattern\n"));

    *step2_nbrs_ = (int **) TSP_allocate_mem(sizeof(int *) * log_p_of_k);

    for (i = 0; i < log_p_of_k; i++) {
        (*step2_nbrs_)[i] = (int *) TSP_allocate_mem(sizeof(int) * (k - 1));
    }

    int **step2_nbrs = *step2_nbrs_;
    int *step1_recvfrom = *step1_recvfrom_ = (int *) TSP_allocate_mem(sizeof(int) * (k - 1));
    *step2_nphases = log_p_of_k;

    rem = nranks - p_of_k;
    /*rem is the number of ranks that do not particpate in Step 2
     * We need to identify the non-participating ranks.
     * The first T ranks are divided into sets of k consecutive ranks each.
     * From each of these sets, the first k-1 ranks are
     * the non-participatig ranks while the last rank participates in Step 2.
     * The non-participating ranks send their data to the participating rank
     * in their set.
     */
    T = (rem * k) / (k - 1);
    *T_ = T;
    *p_of_k_ = p_of_k;


    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"step 1 nbr calculation started. T is %d \n", T));
    *step1_nrecvs = 0;
    *step1_sendto = -1;

    /* Step 1 */
    if (rank < T) {
        if (rank % k != (k - 1)) {
            /* I am a non-participating rank */
            *step1_sendto = rank + (k - 1 - rank % k);
            /* partipating rank to send the data to */

            if (*step1_sendto > T - 1)
                *step1_sendto = T;
            /*if the corresponding participating rank is not in T,
             *
             * then send to the Tth rank to preserve non-commutativity */
            newrank = -1;
            /*tag this rank as non-participating */
        } else {
            /* receiver rank */
            for (i = 0; i < k - 1; i++) {
                step1_recvfrom[i] = rank - i - 1;
            }

            *step1_nrecvs = k - 1;
            /*this is the new rank in the set of participating ranks */
            newrank = rank / k;
        }
    } else {
        /* rank >= T */
        newrank = rank - rem;

        if (rank == T && (T - 1) % k != k - 1 && T >= 1) {
            int nsenders = (T - 1) % k + 1;
            /*number of ranks sending to me */

            for (j = nsenders - 1; j >= 0; j--) {
                step1_recvfrom[nsenders - 1 - j] = T - nsenders + j;
            }

            *step1_nrecvs = nsenders;
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"step 1 nbr computation completed\n"));

    /*Step 2 */
    if (*step1_sendto == -1) {
        /* calulate step2_nbrs only for participating ranks */
        int *digit = (int *) TSP_allocate_mem(sizeof(int) * log_p_of_k);
        /*calculate the digits in base k representation of newrank */
        for (i = 0; i < log_p_of_k; i++)
            digit[i] = 0;
        int temprank = newrank, index = 0, remainder;
        while (temprank != 0) {
            remainder = temprank % k;
            temprank = temprank / k;
            digit[index] = remainder;
            index++;
        }

        int mask = 0x1;
        int phase = 0, cbit, cnt, nbr, power;
        while (mask < p_of_k) {
            cbit = digit[phase];
            /*phase_th digit changes in this phase, obtain its original value */
            cnt = 0;
            for (i = 0; i < k; i++) {
                /*there are k-1 neighbors */
                if (i != cbit) {
                    /*do not generate yourself as your nieighbor */
                    digit[phase] = i;
                    /*this gets us the base k representation of the neighbor */

                    /*calculate the base 10 value of the neighbor rank */
                    nbr = 0;
                    power = 1;
                    for (j = 0; j < log_p_of_k; j++) {
                        nbr += digit[j] * power;
                        power *= k;
                    }

                    /*calculate its real rank and store it */
                    step2_nbrs[phase][cnt] =
                        (nbr < rem / (k - 1)) ? (nbr * k) + (k - 1) : nbr + rem;
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"step2_nbrs[%d][%d] is %d \n", phase, cnt, step2_nbrs[phase][cnt]));
                    cnt++;
                }
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"step 2, phase %d nbr calculation completed\n", phase));
            digit[phase] = cbit;
            phase++;
            mask *= k;
        }

        TSP_free_mem(digit);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_BARRIER_DISSEM);

    return 0;
}

MPL_STATIC_INLINE_PREFIX int
COLL_sched_allreduce_recexch(const void *sendbuf,
                             void *recvbuf,
                             int count,
                             COLL_dt_t datatype,
                             COLL_op_t op,
                             int tag,
                             COLL_comm_t * comm,
                             int k, TSP_sched_t * s, int per_nbr_buffer, int finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLRED_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLRED_RECEXCH);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"In allreduce recexch with per_nbr_buffer %d \n", per_nbr_buffer));

    int is_inplace, is_contig, i, j, p;
    int dtcopy_id = -1;
    TSP_sched_t *sched = s;
    TSP_comm_t *tsp_comm = &comm->tsp_comm;

    size_t type_size, extent, lb;
    int is_commutative;
    TSP_opinfo(op, &is_commutative);
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    is_inplace = TSP_isinplace((void *) sendbuf);
    int nranks = TSP_size(tsp_comm);
    int rank = TSP_rank(tsp_comm);
    /*if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace && count > 0)
            TSP_dtcopy_nb(recvbuf, count, datatype, sendbuf, count, datatype, sched, 0, NULL);
        return 0;
    }

    int step1_sendto = -1;
    int step2_nphases;
    int step1_nrecvs;

    /*COLL_get_neighbors_recexch function allocates memory
     * to these pointers */
    int *step1_recvfrom;
    int **step2_nbrs;
    int p_of_k, T;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Beforeinitial dt copy\n"));
    /*get the neighbors */
    COLL_get_neighbors_recexch(rank, nranks, &k, &step1_sendto,
                               &step1_recvfrom, &step1_nrecvs,
                               &step2_nbrs, &step2_nphases, &p_of_k, &T);
    bool in_step2 = (step1_sendto == -1);

    void *tmp_buf = TSP_allocate_buffer(count * extent, sched);

    if (in_step2 && !is_inplace && count > 0) {
        /*copy the data to recvbuf but only if you are a rank participating in Step 2 */
        dtcopy_id = TSP_dtcopy_nb(recvbuf, count, datatype,
                                  sendbuf, count, datatype, sched, 0, NULL);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"After initial dt copy\n"));
    int *send_id = (int *) TSP_allocate_mem(sizeof(int) * k);   /*to store send ids */
    int *reduce_id = (int *) TSP_allocate_mem(sizeof(int) * k); /*to store recv_reduce ids */
    /*Step 1 */
    if (!in_step2) {
        /*non-participating rank sends the data to a partcipating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        TSP_send(buf_to_send, count, datatype, step1_sendto, tag, tsp_comm, sched, 0, NULL);
    }

    void **step1_recvbuf = (void **) TSP_allocate_mem(sizeof(void *) * step1_nrecvs);
    if (per_nbr_buffer != 1 && step1_nrecvs > 0)
        step1_recvbuf[0] = TSP_allocate_buffer(count * extent, sched);
    int buf;
    int *recv_id = (int*) TSP_allocate_mem(sizeof(int) * step2_nphases * (k-1));
    int nvtcs;
    for (i = 0; i < step1_nrecvs; i++) {
        /*participating rank gets data from non-partcipating ranks */
        buf = (per_nbr_buffer == 1) ? i : 0;
        int vtcs[2];
        if (per_nbr_buffer == 1)
            step1_recvbuf[buf] = TSP_allocate_buffer(count * extent, sched);
        /* recv dependencies */
        if (i == 0 || per_nbr_buffer == 1 || count == 0)
            nvtcs = 0;
        else {
            nvtcs = 1;
            vtcs[0] = reduce_id[i - 1];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"step1 recv depend on reduce_id[%d] %d \n", i-1, reduce_id[i - 1]));
        }
        recv_id[i] = TSP_recv(step1_recvbuf[buf], count, datatype,
                           step1_recvfrom[i], tag, tsp_comm, sched, nvtcs, vtcs);
        if (count != 0) {       /* Reduce only if data is present */
            /* reduce dependencies */
            if (is_commutative) {
                if (!is_inplace) {
                    /*wait for the data to be copied to recvbuf */
                    nvtcs = 2;
                    vtcs[0] = dtcopy_id;
                    vtcs[1] = recv_id[i];
                } else {
                    /*i have no dependency */
                    nvtcs = 1;
                    vtcs[0] = recv_id[i];
                }
            } else {
                //if not commutative
                if (i == 0 && is_inplace) {
                    /*if data is inplace, no need to wait */
                    nvtcs = 1;
                    vtcs[0] = recv_id[i];
                } else {
                    /*wait for datacopy to complete if i==0 else wait for previous recv_reduce */
                    nvtcs = 2;
                    if (i == 0)
                        vtcs[0] = dtcopy_id;
                    else
                        vtcs[0] = reduce_id[i - 1];
                    vtcs[1] = recv_id[i];
                }
            }
            reduce_id[i] = TSP_reduce_local(step1_recvbuf[buf], recvbuf, count,
                                            datatype, op, TSP_FLAG_REDUCE_L, sched, nvtcs, vtcs);
        }
    }
    int fence_id = TSP_fence(sched);

    /*Step 2 */
    int myidx, nbr, phase;
    int *vtcs;
    if(step2_nphases > 2 && count == 0)
        vtcs = TSP_allocate_mem(sizeof(int) * (step2_nphases) * k);
    else
        vtcs = TSP_allocate_mem(sizeof(int) * 2 * k);
    /*used for specifying graph dependencies */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"After step1"));

    void **nbr_buffer = (void **) TSP_allocate_mem(sizeof(void *) * step2_nphases * (k - 1));
    buf = 0;
    if (step2_nphases > 0)
        nbr_buffer[0] = TSP_allocate_buffer(count * extent, sched);
    for (i = 0; i < step2_nphases; i++) {
        for (j = 0; j < (k - 1); j++, buf++) {
            if (buf == 0)
                /*memory is already allocated for the first child above */
                continue;
            else {
                if (per_nbr_buffer) {
                    nbr_buffer[buf] = TSP_allocate_buffer(count * extent, sched);
                } else
                    nbr_buffer[buf] = nbr_buffer[0];
            }
        }
    }
    buf = 0;
    int counter = 0;
    for (phase = 0; phase < step2_nphases && step1_sendto == -1; phase++) {
        if (!is_commutative) {
            /*sort the neighbor list so that receives can be posted in order */
            qsort(step2_nbrs[phase], k - 1, sizeof(int), intcmpfn);
        }
        /*copy the data to a temporary buffer so that sends ands recvs
         * can be posted simultaneosly */
        if (count != 0) {
            if (phase == 0) {
                /*wait for Step 1 to complete */
                nvtcs = 1;
                vtcs[0] = fence_id;
            } else {
                nvtcs = k - 1;
                for (i = 0; i < k - 1; i++)
                    vtcs[i] = send_id[i];
                if (is_commutative && per_nbr_buffer == 1) {
                    /*wait for all the previous recv_reduce to complete */
                    for (i = 0; i < k - 1; i++)
                        vtcs[i + nvtcs] = reduce_id[i];
                    nvtcs += k - 1;
                } else { /* if it is not commutative or if there is only one recv buffer */
                    /*just wait for the last recv_reduce to complete as recv_reduce happen in order */
                    vtcs[nvtcs] = reduce_id[k - 2];
                    nvtcs += 1;
                    /*wait for the previous sends to complete and the last recv_reduce */
                }
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"dtcopy from recvbuf to tmpbuf. nvtcs %d counter %d \n", nvtcs, counter));
            dtcopy_id = TSP_dtcopy_nb(tmp_buf, count, datatype,
                                      recvbuf, count, datatype, sched, nvtcs, vtcs);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Step 1: data copy scheduled\n"));
        }
        /*myidx is the index in the neighbors list such that
         * all neighbors before myidx have ranks less than my rank
         * and neighbors at and after myidx have rank greater than me.
         * This is useful only in the non-commutative case. */
        myidx = 0;
        /*send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            if (count == 0){
                if(phase==0){
                    nvtcs = 1;
                    vtcs[0] = fence_id;
                    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"count 0. phase is also 0 \n"));
                }
                else{
                  /*If count is zero, no reduce operation. So just depend on all prev recvs */
                   for(p=0; p<phase; p++)
                       for (j = 0; j < k - 1; j++)
                           vtcs[p*(k-1)+j] = recv_id[p*(k-1)+j];
                   nvtcs = phase*(k-1);
                   MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"count 0. Depend on all previous %d recvs. \n", nvtcs));
                }
            }
            else {
                nvtcs = 1;
                vtcs[0] = dtcopy_id;
            }
            nbr = step2_nbrs[phase][i];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"sending data to %d. I'm dependant %d nvtcs \n", nbr, nvtcs));
            send_id[i] = TSP_send(tmp_buf, count, datatype, nbr, tag, tsp_comm, sched, nvtcs, vtcs);
            if (rank > nbr) {
                myidx = i + 1;
            }
        }
        //if(per_nbr_buffer==1)
         counter = 0;
        /*receive from the neighbors to the left. Need datacopy here */
        for (i = myidx - 1; i >= 0; i--, buf++) {
            nbr = step2_nbrs[phase][i];
            if (per_nbr_buffer == 1 || (phase == 0 && counter == 0) || count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = (counter==0)? reduce_id[k-2] : reduce_id[counter-1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"receiving data to %d. I'm dependant nvtcs %d vtcs %d counter  %d \n", nbr, nvtcs, vtcs[0], counter));
            }
            recv_id[phase*(k-1)+i] = TSP_recv(nbr_buffer[buf], count, datatype, nbr,
                               tag, tsp_comm, sched, nvtcs, vtcs);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[phase*(k-1)+i];
                if (counter == 0 || is_commutative){
                    vtcs[1] = dtcopy_id;
                    nvtcs += 1;
                }
                else{
                    nvtcs += 1;
                    vtcs[1] = reduce_id[counter-1];
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"reducing data with count %d dependant %d nvtcs counter %d \n", count, nvtcs, counter));
                reduce_id[counter++] = TSP_reduce_local(nbr_buffer[buf],
                                                        recvbuf, count,
                                                        datatype, op,
                                                        TSP_FLAG_REDUCE_L, sched, nvtcs, vtcs);
            }

        }

        /*receive from the neighbors to the right */
        for (i = myidx; i < k - 1; i++, buf++) {
            nbr = step2_nbrs[phase][i];
            if (per_nbr_buffer == 1 || (phase == 0 && counter ==0) || count == 0)
                nvtcs = 0;
            else {
                nvtcs = 1;
                vtcs[0] = (counter==0)? reduce_id[k-2]:reduce_id[counter - 1];
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"receiving data to %d. I'm dependant nvtcs %d vtcs %d counter %d \n", nbr, nvtcs, vtcs[0], counter));
            }
            recv_id[phase*(k-1)+i] = TSP_recv(nbr_buffer[buf], count, datatype, nbr,
                                             tag, tsp_comm, sched, nvtcs, vtcs);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[phase*(k-1)+i];
                if (counter == 0 || is_commutative){
                    vtcs[1] = dtcopy_id;
                    nvtcs += 1;
                }
                else{
                    vtcs[1] = reduce_id[counter-1];
                    nvtcs += 1;
                }
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"reducing data with count %d dependant %d nvtcs coutner %d \n", count, nvtcs, counter));
                reduce_id[counter++] = TSP_reduce_local(nbr_buffer[buf], recvbuf, count,
                                                        datatype, op, TSP_FLAG_REDUCE_R, sched,
                                                        nvtcs, vtcs);
            }
        }
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"After step2\n"));

    /*Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (step1_sendto != -1) {
        TSP_recv(recvbuf, count, datatype, step1_sendto, tag, tsp_comm, sched, 0, NULL);
        //TSP_free_mem(tmp_buf);
    } else {    /* free temporary buffer after it is used in last send */
        //TSP_free_mem_nb(tmp_buf,sched,k-1,id+1);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        if (count == 0 ){
            nvtcs = step2_nphases*(k-1);
            for(p = 0; p< step2_nphases; p++)
              for (j = 0; j < k - 1; j++)
                 vtcs[p*(k-1)+j] = recv_id[p*(k-1)+j];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"count 0. Step 3. depends on all prev recvs %d \n", nvtcs));
        }
        else if (is_commutative && per_nbr_buffer == 1) {
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
        TSP_send(recvbuf, count, datatype, step1_recvfrom[i], tag, tsp_comm, sched, nvtcs, vtcs);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"After step3\n"));
    /*free all allocated memory for storing nbrs */
    for (i = 0; i < step2_nphases; i++)
        TSP_free_mem(step2_nbrs[i]);
    TSP_free_mem(step2_nbrs);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"freed step2_nbrs\n"));
    TSP_free_mem(step1_recvfrom);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"freed step1_recvfrom\n"));

    TSP_free_mem(send_id);
    TSP_free_mem(recv_id);
    TSP_free_mem(reduce_id);
    TSP_free_mem(vtcs);
    TSP_free_mem(nbr_buffer);
    TSP_free_mem(step1_recvbuf);

    if (finalize) {
        TSP_sched_commit(sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLRED_RECEXCH);

    return 0;
}

MPL_STATIC_INLINE_PREFIX int
COLL_sched_allgather_recexch_data_exchange(int rank,
                                           int nranks,
                                           int k,
                                           int p_of_k,
                                           int log_pofk,
                                           int T,
                                           void *recvbuf,
                                           COLL_dt_t recvtype,
                                           size_t recv_extent,
                                           int recvcount,
                                           int tag, COLL_comm_t * comm, TSP_sched_t * s)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_DATA_EXCHANGE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_DATA_EXCHANGE);

    int partner, offset, count, send_offset, recv_offset;
    /* calculate send offset to receive the correct data from bit reversed partner */
    partner = reverse_digits(rank, nranks, k, p_of_k, log_pofk, T);     /*get the partner with whom I should exchange data */
    allgather_get_offset_and_count(rank, 0, k, nranks, &count, &offset);
    send_offset = offset * recv_extent * recvcount;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"data exchange with %d send_offset %d count %d \n", partner, send_offset, count));
    /* send my data to my bit reverse partner */
    TSP_send(((char *) recvbuf + send_offset), count * recvcount, recvtype, partner,
             tag, &comm->tsp_comm, s, 0, NULL);

    /* calculate recv offset to receive the correct data from bit reversed partner */
    allgather_get_offset_and_count(partner, 0, k, nranks, &count, &offset);
    recv_offset = offset * recv_extent * recvcount;
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"data exchange with %d recv_offset %d count %d \n", partner, recv_offset, count));
    /* Recv data my bit reverse partner */
    TSP_recv(((char *) recvbuf + recv_offset), count * recvcount, recvtype,
             partner, tag, &comm->tsp_comm, s, 0, NULL);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_DATA_EXCHANGE);

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
COLL_sched_allgather_recexch_step1(int step1_sendto,
                                   int *step1_recvfrom,
                                   int step1_nrecvs,
                                   int is_inplace,
                                   int rank,
                                   int tag,
                                   const void *sendbuf,
                                   void *recvbuf,
                                   size_t recv_extent,
                                   int recvcount,
                                   COLL_dt_t recvtype,
                                   int n_invtcs, int *invtx, COLL_comm_t * comm, TSP_sched_t * s)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP1);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP1);

    int send_offset, recv_offset, i;
    if (step1_sendto != -1) {   /*non-participating rank sends the data to a partcipating rank */
        send_offset = rank * recv_extent * recvcount;
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = ((char *) recvbuf + send_offset);
        else
            buf_to_send = (void *) sendbuf;
        TSP_send(buf_to_send, recvcount, recvtype, step1_sendto, tag, &comm->tsp_comm, s, 0, NULL);

    } else {
        for (i = 0; i < step1_nrecvs; i++) {    /*participating rank gets the data from non-participating rank */
            recv_offset = step1_recvfrom[i] * recv_extent * recvcount;
            TSP_recv(((char *) recvbuf + recv_offset), recvcount, recvtype,
                     step1_recvfrom[i], tag, &comm->tsp_comm, s, n_invtcs, invtx);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP1);

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
COLL_sched_allgather_recexch_step2(int step1_sendto,
                                   int step2_nphases,
                                   int **step2_nbrs,
                                   int rank,
                                   int nranks,
                                   int k,
                                   int p_of_k,
                                   int log_pofk,
                                   int T,
                                   int *nrecvs_,
                                   int **recv_id_,
                                   int tag,
                                   void *recvbuf,
                                   size_t recv_extent,
                                   int recvcount,
                                   COLL_dt_t recvtype,
                                   COLL_comm_t * comm, TSP_sched_t * s, int dist_halving)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP2);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP2);

    int phase, i, j, count, nbr, send_offset, recv_offset, offset, rank_for_offset;
    int *recv_id = *recv_id_;
    int nrecvs = 0;
    if (dist_halving == 1) {
        phase = step2_nphases - 1;
    } else {
        phase = 0;
    }
    for (j = 0; j < step2_nphases && step1_sendto == -1; j++) {
        /*send data to all the neighbors */
        for (i = 0; i < k - 1; i++) {
            nbr = step2_nbrs[phase][i];
            if (dist_halving == 1)
                rank_for_offset = reverse_digits(rank, nranks, k, p_of_k, log_pofk, T);
            else
                rank_for_offset = rank;
            allgather_get_offset_and_count(rank_for_offset, j, k, nranks, &count, &offset);
            send_offset = offset * recv_extent * recvcount;
            TSP_send(((char *) recvbuf + send_offset), count * recvcount, recvtype,
                     nbr, tag, &comm->tsp_comm, s, nrecvs, recv_id);
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"phase %d nbr is %d send offset %d count %d depend on %d \n", phase, nbr,
                     send_offset, count, ((k - 1) * j)));
        }
        /*receive from the neighbors */
        for (i = 0; i < k - 1; i++) {
            nbr = step2_nbrs[phase][i];
            if (dist_halving == 1)
                rank_for_offset = reverse_digits(nbr, nranks, k, p_of_k, log_pofk, T);
            else
                rank_for_offset = nbr;
            allgather_get_offset_and_count(rank_for_offset, j, k, nranks, &count, &offset);
            recv_offset = offset * recv_extent * recvcount;
            recv_id[j * (k - 1) + i] =
                TSP_recv(((char *) recvbuf + recv_offset), count * recvcount, recvtype, nbr, tag,
                         &comm->tsp_comm, s, 0, NULL);
            nrecvs++;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"phase %d recv from %d recv offset %d cur_count %d recv returns id[%d] %d\n",
                     phase, nbr, recv_offset, count, j * (k - 1) + i, recv_id[j * (k - 1) + i]));
        }
        if (dist_halving == 1)
            phase--;
        else
            phase++;
    }
    *nrecvs_ = nrecvs;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP2);

    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int
COLL_sched_allgather_recexch_step3(int step1_sendto,
                                   int *step1_recvfrom,
                                   int step1_nrecvs,
                                   int step2_nphases,
                                   void *recvbuf,
                                   int recvcount,
                                   int nranks,
                                   int k,
                                   int nrecvs,
                                   int *recv_id,
                                   int tag, COLL_dt_t recvtype, COLL_comm_t * comm, TSP_sched_t * s)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP3);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP3);

    int i;
    if (step1_sendto != -1) {
        TSP_recv(recvbuf, recvcount * nranks, recvtype, step1_sendto, tag, &comm->tsp_comm, s, 0,
                 NULL);
    }

    for (i = 0; i < step1_nrecvs; i++) {
        TSP_send(recvbuf, recvcount * nranks, recvtype, step1_recvfrom[i],
                 tag, &comm->tsp_comm, s, nrecvs, recv_id);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH_STEP3);

    return MPI_SUCCESS;
}

/* Allgather recursive koupling distance halving */
MPL_STATIC_INLINE_PREFIX int
COLL_sched_allgather_recexch(const void *sendbuf,
                             int sendcount,
                             COLL_dt_t sendtype,
                             void *recvbuf,
                             int recvcount,
                             COLL_dt_t recvtype,
                             int tag, COLL_comm_t * comm, TSP_sched_t * s, int k, int dist_halving)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH);

    int is_inplace, is_contig, i;
    int nranks = TSP_size(&comm->tsp_comm);
    int rank = TSP_rank(&comm->tsp_comm);
    size_t recv_type_size, recv_extent, recv_lb;
    is_inplace = TSP_isinplace((void *) sendbuf);
    TSP_dtinfo(recvtype, &is_contig, &recv_type_size, &recv_extent, &recv_lb);

    int step1_sendto = -1, step2_nphases, step1_nrecvs, p_of_k, T;

    /*COLL_get_neighbors_recexch function allocates memory
     * to these pointers */
    int *step1_recvfrom;
    int **step2_nbrs;
    /* Find out if I'm a participating rank and also get the neighbors to whom I should send and receive data in step1 */
    COLL_get_neighbors_recexch(rank, nranks, &k, &step1_sendto, &step1_recvfrom, &step1_nrecvs,
                               &step2_nbrs, &step2_nphases, &p_of_k, &T);
    bool is_instep2 = (step1_sendto == -1);
    int log_pofk = step2_nphases;
    int dtcopy_id;
    int n_invtcs = 0;
    int invtx;
    if (!is_inplace && is_instep2) {
        dtcopy_id = TSP_dtcopy_nb((char *) recvbuf + rank * recv_extent * recvcount, recvcount, recvtype, sendbuf, recvcount, recvtype, s, 0, NULL);    /*copy the data to recvbuf but only if you are a rank participating in Step 2 */
    }
    /*step 1 */
    n_invtcs = (is_inplace) ? 0 : 1;
    invtx = dtcopy_id;
    COLL_sched_allgather_recexch_step1(step1_sendto, step1_recvfrom, step1_nrecvs, is_inplace, rank,
                                       tag, sendbuf, recvbuf, recv_extent, recvcount, recvtype,
                                       n_invtcs, &invtx, comm, s);
    TSP_wait(s);
    /* Exchange the data with digit reversed partners only for distance halving so that all data is in proper order in the final step */
    if (dist_halving == 1) {
        if (step1_sendto == -1) {
            COLL_sched_allgather_recexch_data_exchange(rank, nranks, k, p_of_k, log_pofk, T,
                                                       recvbuf, recvtype, recv_extent, recvcount,
                                                       tag, comm, s);
            TSP_wait(s);
        }
    }
    int nrecvs;
    int *recv_id = (int *) TSP_allocate_mem(sizeof(int) * ((step2_nphases * (k - 1)) + 1));
    /* step2 */
    COLL_sched_allgather_recexch_step2(step1_sendto, step2_nphases, step2_nbrs, rank, nranks, k,
                                       p_of_k, log_pofk, T, &nrecvs, &recv_id, tag, recvbuf,
                                       recv_extent, recvcount, recvtype, comm, s, dist_halving);
    /*Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    COLL_sched_allgather_recexch_step3(step1_sendto, step1_recvfrom, step1_nrecvs, step2_nphases,
                                       recvbuf, recvcount, nranks, k, nrecvs, recv_id, tag,
                                       recvtype, comm, s);
    /* free the memory */
    for (i = 0; i < step2_nphases; i++)
        TSP_free_mem(step2_nbrs[i]);
    TSP_free_mem(step2_nbrs);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"freed step2_nbrs\n"));
    TSP_free_mem(step1_recvfrom);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"freed step1_recvfrom\n"));
    TSP_free_mem(recv_id);
    TSP_sched_commit(s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_SCHED_ALLGATHER_RECEXCH);

    return 0;
}
