/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "recexchalgo.h"
#include "iallreduce_tsp_recursive_exchange_common.h"

/* Allreduce single buffer and multiple buffer without dtcopy algorithms are an optimization to the existing
 * allreduce recexch algorithms. These two algorithms are decoupled and written as separate algorithms
 * for readability */

/* Routine to schedule a recursive exchange based allreduce using single receive buffer without
 * data copy in step2. Similar to other recursive exchange algorithms, this algorithm also involves "k"
 * partners in every communication phase. Removing the data copy in step2 improves performance but at the cost
 * of increased dependencies. */
static int single_buffer_without_dtcopy_recexch(const void *sendbuf, void *recvbuf,
                                                MPI_Aint count, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm, int k,
                                                MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_Assert(k == 2);        /* This algorithm will deadlock if the exchange involves more than 2 partners */

    int is_inplace, i;
    int dtcopy_id = -1;
    size_t extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int nranks, rank;
    int p_of_k, T;
    int nvtcs, *vtcs;
    int nbr, phase;
    int send_id, reduce_id, recv_id;
    void *step1_recvbuf = NULL;
    void *nbr_buffer = NULL;
    int tag;

    MPIR_FUNC_ENTER;

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace) {
            int dummy_id;
            mpi_errno =
                MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched,
                                         0, NULL, &dummy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        goto fn_exit;
    }

    MPII_Recexchalgo_t recexch;
    MPII_Recexchalgo_start(rank, nranks, k, &recexch);

    vtcs = MPL_malloc(sizeof(int) * (recexch.step2_nphases) * 2, MPL_MEM_COLL); /* to store graph dependencies */
    MPIR_Assert(vtcs != NULL);  /* If memory allocation for vtcs fails, MPI will abort. Else MPI would be inconsistent */


    if (recexch.in_step2 && !is_inplace) {      /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL, &dtcopy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy"));
    }

    /* Step 1 */
    if (!recexch.in_step2) {
        /* non-participating rank sends the data to a participating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        int dummy_id;
        mpi_errno =
            MPIR_TSP_sched_isend(buf_to_send, count, datatype, recexch.step1_sendto, tag, comm,
                                 sched, 0, NULL, &dummy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {    /* Step 2 participating rank */
        step1_recvbuf = MPIR_TSP_sched_malloc(count * extent, sched);
        MPIR_Assert(step1_recvbuf != NULL);

        for (i = 0; i < recexch.step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            /* recv dependencies */
            nvtcs = 0;
            if (i != 0) {
                vtcs[nvtcs++] = reduce_id;
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "step1 recv depend on reduce_id %d", reduce_id));
            }
            mpi_errno = MPIR_TSP_sched_irecv(step1_recvbuf, count, datatype,
                                             recexch.step1_recvfrom[i], tag, comm, sched, nvtcs,
                                             vtcs, &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            /* setup reduce dependencies */
            nvtcs = 1;
            vtcs[0] = recv_id;
            if (i == 0 && !is_inplace)
                vtcs[nvtcs++] = dtcopy_id;
            mpi_errno = MPIR_TSP_sched_reduce_local(step1_recvbuf, recvbuf, count,
                                                    datatype, op, sched, nvtcs, vtcs, &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    /* Step 2 */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Start Step2"));

    /* allocate memory for receive buffers */
    if (recexch.in_step2) {
        nbr_buffer = step1_recvbuf;
        MPIR_Assert(nbr_buffer != NULL);
    }

    for (phase = 0; phase < recexch.step2_nphases && recexch.step1_sendto == -1; phase++) {

        nbr = recexch.step2_nbrs[phase][0];
        /* send data to all the neighbors */
        nvtcs = 0;
        if ((phase == 0 && recexch.step1_nrecvs > 0) || phase != 0) {
            vtcs[nvtcs++] = reduce_id;
        } else if (phase == 0 && recexch.in_step2 && !is_inplace) {
            vtcs[nvtcs++] = dtcopy_id;
        }

        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "sending data to %d. I'm dependent on nvtcs %d", nbr,
                         nvtcs));
        mpi_errno =
            MPIR_TSP_sched_isend(recvbuf, count, datatype, nbr, tag, comm, sched, nvtcs, vtcs,
                                 &send_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        /* receive and reduce data from the neighbors to the left */
        nvtcs = 0;
        if ((phase == 0 && recexch.step1_nrecvs > 0) || phase != 0) {
            vtcs[nvtcs++] = reduce_id;
        }
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "receiving data to %d. I'm dependent on nvtcs %d vtcs %d",
                         nbr, nvtcs, vtcs[0]));
        mpi_errno =
            MPIR_TSP_sched_irecv(nbr_buffer, count, datatype, nbr, tag, comm, sched, nvtcs, vtcs,
                                 &recv_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        nvtcs = 2;
        vtcs[0] = recv_id;
        vtcs[1] = send_id;

        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "reducing data with count %ld dependent %d nvtcs", count, nvtcs));
        if (is_commutative) {
            mpi_errno =
                MPIR_TSP_sched_reduce_local(nbr_buffer, recvbuf, count, datatype, op,
                                            sched, nvtcs, vtcs, &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        } else {
            mpi_errno =
                MPIR_TSP_sched_reduce_local(recvbuf, nbr_buffer, count, datatype, op,
                                            sched, nvtcs, vtcs, &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            mpi_errno =
                MPIR_TSP_sched_localcopy(nbr_buffer, count, datatype, recvbuf, count,
                                         datatype, sched, 1, &reduce_id, &reduce_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After Step 2"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (recexch.step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        int dummy_id;
        mpi_errno =
            MPIR_TSP_sched_irecv(recvbuf, count, datatype, recexch.step1_sendto, tag, comm, sched,
                                 0, NULL, &dummy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {
        /* Wait for the last reduce to complete */
        nvtcs = 1;
        vtcs[0] = reduce_id;
        for (i = 0; i < recexch.step1_nrecvs; i++) {
            int dummy_id;
            mpi_errno =
                MPIR_TSP_sched_isend(recvbuf, count, datatype, recexch.step1_recvfrom[i], tag, comm,
                                     sched, nvtcs, vtcs, &dummy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Done Step 3"));

    MPII_Recexchalgo_finish(&recexch);
    MPL_free(vtcs);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Routine to schedule a recursive exchange based allreduce using separate receive buffer
 * for each partner and in each phase and without data copy in step2.
 * Removing the data copy improves performance but at the cost
 * of increased dependencies. */
static int multiple_buffer_without_dtcopy_recexch(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm, int k,
                                                  MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int is_inplace, i, j;
    int dtcopy_id = -1;
    size_t extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int nranks, rank;
    int p_of_k, T;
    int buf = 0;
    int nvtcs, step1_id, *recv_id, *vtcs;
    int myidx, nbr, phase;
    int counter = 0;
    int *reduce_id, imcast_id;
    void **step1_recvbuf = NULL;
    void **nbr_buffer = NULL;
    int tag;

    MPIR_FUNC_ENTER;

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* if there is only 1 rank, copy data from sendbuf
     * to recvbuf and exit */
    if (nranks == 1) {
        if (!is_inplace) {
            int dummy_id;
            mpi_errno =
                MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype, sched,
                                         0, NULL, &dummy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        goto fn_exit;
    }

    MPII_Recexchalgo_t recexch;
    MPII_Recexchalgo_start(rank, nranks, k, &recexch);

    reduce_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);      /* to store reduce vertex ids */
    recv_id = (int *) MPL_malloc(sizeof(int) * recexch.step2_nphases * (k - 1), MPL_MEM_COLL);  /* to store receive vertex ids */
    vtcs = MPL_malloc(sizeof(int) * (recexch.step2_nphases) * k, MPL_MEM_COLL); /* to store graph dependencies */
    /* If memory allocation fails here, MPI will abort. Else MPI would be inconsistent */
    MPIR_Assert(reduce_id != NULL && recv_id != NULL && vtcs != NULL);


    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Beforeinitial dt copy"));
    if (recexch.in_step2 && !is_inplace) {      /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL, &dtcopy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After initial dt copy"));

    /* Step 1 */
    if (!recexch.in_step2) {
        /* non-participating rank sends the data to a participating rank */
        void *buf_to_send;
        if (is_inplace)
            buf_to_send = recvbuf;
        else
            buf_to_send = (void *) sendbuf;
        int dummy_id;
        mpi_errno =
            MPIR_TSP_sched_isend(buf_to_send, count, datatype, recexch.step1_sendto, tag, comm,
                                 sched, 0, NULL, &dummy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {    /* Step 2 participating rank */
        step1_recvbuf = (void **) MPL_malloc(sizeof(void *) * recexch.step1_nrecvs, MPL_MEM_COLL);
        MPIR_Assert(step1_recvbuf != NULL);

        for (i = 0; i < recexch.step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            step1_recvbuf[i] = MPIR_TSP_sched_malloc(count * extent, sched);

            /* recv dependencies */
            nvtcs = 0;
            mpi_errno = MPIR_TSP_sched_irecv(step1_recvbuf[i], count, datatype,
                                             recexch.step1_recvfrom[i], tag, comm, sched, nvtcs,
                                             vtcs, &recv_id[i]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            /* setup reduce dependencies */
            nvtcs = 1;
            vtcs[0] = recv_id[i];
            if (is_commutative) {
                if (!is_inplace) {      /* wait for the data to be copied to recvbuf */
                    vtcs[nvtcs++] = dtcopy_id;
                }
            } else {    /* if not commutative */
                /* wait for datacopy to complete if i==0 && !is_inplace else wait for previous reduce */
                if (i == 0 && !is_inplace)
                    vtcs[nvtcs++] = dtcopy_id;
                else if (i != 0)
                    vtcs[nvtcs++] = reduce_id[i - 1];
            }
            mpi_errno = MPIR_TSP_sched_reduce_local(step1_recvbuf[i], recvbuf, count,
                                                    datatype, op, sched, nvtcs, vtcs,
                                                    &reduce_id[i]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    mpi_errno = MPIR_TSP_sched_sink(sched, &step1_id);  /* sink for all the tasks up to end of Step 1 */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Step 2 */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Start Step2"));

    /* allocate memory for receive buffers */
    nbr_buffer =
        (void **) MPL_malloc(sizeof(void *) * recexch.step2_nphases * (k - 1), MPL_MEM_COLL);
    MPIR_Assert(nbr_buffer != NULL);
    buf = 0;
    for (i = 0; i < recexch.step2_nphases; i++) {
        for (j = 0; j < (k - 1); j++, buf++) {
            nbr_buffer[buf] = MPIR_TSP_sched_malloc(count * extent, sched);
        }
    }

    buf = 0;
    for (phase = 0; phase < recexch.step2_nphases && recexch.step1_sendto == -1; phase++) {
        if (!is_commutative) {
            /* sort the neighbor list so that receives can be posted in order */
#if defined(HAVE_QSORT)
            qsort(recexch.step2_nbrs[phase], k - 1, sizeof(int), MPII_Algo_compare_int);
#else
            MPIR_insertion_sort(recexch.step2_nbrs[phase], k - 1);
#endif
        }
        /* myidx is the index in the neighbors list such that
         * all neighbors before myidx have ranks less than my rank
         * and neighbors at and after myidx have rank greater than me.
         * This is useful only in the non-commutative case. */
        myidx = 0;
        /* send data to all the neighbors */
        if (phase == 0) {
            nvtcs = 1;
            vtcs[0] = step1_id;
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "phase is 0"));
        } else if (is_commutative) {    /* wait for all the previous receives to have completed */
            nvtcs = 0;
            for (j = 0; j < k - 1; j++)
                vtcs[nvtcs++] = reduce_id[j];
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Depend on all reduces (%d)", nvtcs));
        } else {        /* if it is not commutative or if there is only one recv buffer */
            /*just wait for the last recv_reduce to complete as recv_reduce happen in order */
            nvtcs = 1;
            vtcs[0] = reduce_id[k - 2];
        }

        if (!is_commutative)
            for (i = 0; i < k - 1; i++) {
                nbr = recexch.step2_nbrs[phase][i];
                if (rank > nbr) {
                    myidx = i + 1;
                }
            }
        mpi_errno =
            MPIR_TSP_sched_imcast(recvbuf, count, datatype, recexch.step2_nbrs[phase], k - 1, tag,
                                  comm, sched, nvtcs, vtcs, &imcast_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "sending imcast to %d brs. I'm dependent on nvtcs %d",
                         k - 1, nvtcs));

        /* receive and reduce data from the neighbors to the left */
        for (i = myidx - 1, counter = 0; i >= 0; i--, counter++, buf++) {
            nbr = recexch.step2_nbrs[phase][i];
            nvtcs = 0;
            mpi_errno =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs, &recv_id[buf]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            nvtcs = 1;
            vtcs[0] = recv_id[buf];
            /*If no dtcopy, reduce should wait on all previous phase sends to complete */
            if (counter == 0 || is_commutative) {
                vtcs[nvtcs++] = imcast_id;
            } else {
                vtcs[nvtcs++] = reduce_id[counter - 1];
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "reducing data with count %ld dependent %d nvtcs counter %d",
                             count, nvtcs, counter));
            mpi_errno =
                MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                            sched, nvtcs, vtcs, &reduce_id[counter]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        }

        /* receive and reduce data from the neighbors to the right */
        for (i = myidx; i < k - 1; i++, counter++, buf++) {
            nbr = recexch.step2_nbrs[phase][i];
            nvtcs = 0;
            mpi_errno =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs, &recv_id[buf]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            nvtcs = 1;
            vtcs[0] = recv_id[buf];
            /*If no dtcopy, reduce should wait on all previous phase sends to complete */
            if (counter == 0 || is_commutative) {
                vtcs[nvtcs++] = imcast_id;
            } else {
                vtcs[nvtcs++] = reduce_id[counter - 1];
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST,
                             "reducing data with count %ld dependent %d nvtcs counter %d right neighbor",
                             count, nvtcs, counter));
            if (is_commutative) {
                mpi_errno =
                    MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                                sched, nvtcs, vtcs, &reduce_id[counter]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            } else {
                mpi_errno =
                    MPIR_TSP_sched_reduce_local(recvbuf, nbr_buffer[buf], count, datatype, op,
                                                sched, nvtcs, vtcs, &reduce_id[counter]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
                mpi_errno =
                    MPIR_TSP_sched_localcopy(nbr_buffer[buf], count, datatype, recvbuf, count,
                                             datatype, sched, 1, &reduce_id[counter],
                                             &reduce_id[counter]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            }
        }
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "After Step 2"));

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (recexch.step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        int dummy_id;
        mpi_errno =
            MPIR_TSP_sched_irecv(recvbuf, count, datatype, recexch.step1_sendto, tag, comm, sched,
                                 0, NULL, &dummy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {
        for (i = 0; i < recexch.step1_nrecvs; i++) {
            if (is_commutative) {
                /* If commutative, wait for all the prev reduce calls to complete
                 * since they can happen in any order */
                nvtcs = k - 1;
                MPIR_Localcopy(reduce_id, k - 1, MPI_INT, vtcs, k - 1, MPI_INT);
            } else {
                /* If non commutative, wait for the last reduce to complete, as they happen in sequential order */
                nvtcs = 1;
                vtcs[0] = reduce_id[k - 2];
            }
            int dummy_id;
            mpi_errno =
                MPIR_TSP_sched_isend(recvbuf, count, datatype, recexch.step1_recvfrom[i], tag, comm,
                                     sched, nvtcs, vtcs, &dummy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "Done Step 3"));

    MPII_Recexchalgo_finish(&recexch);
    MPL_free(reduce_id);
    MPL_free(recv_id);
    MPL_free(vtcs);
    MPL_free(nbr_buffer);
    if (recexch.in_step2)
        MPL_free(step1_recvbuf);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Routine to schedule a recursive exchange based allreduce */
int MPIR_TSP_Iallreduce_sched_intra_recexch(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                            MPI_Datatype datatype, MPI_Op op,
                                            MPIR_Comm * comm, int per_nbr_buffer, int do_dtcopy,
                                            int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int is_inplace, i, j;
    int dtcopy_id = -1;
    size_t extent;
    MPI_Aint lb, true_extent;
    int is_commutative;
    int nranks, rank;
    int p_of_k, T;
    int buf = 0;
    int nvtcs, step1_id, *recv_id, *vtcs;
    int myidx, nbr, phase;
    int counter = 0;
    int *reduce_id, imcast_id;
    void *tmp_buf;
    void **step1_recvbuf = NULL;
    void **nbr_buffer;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    if (!do_dtcopy) {
        if (!per_nbr_buffer) {
            mpi_errno = single_buffer_without_dtcopy_recexch(sendbuf, recvbuf, count, datatype,
                                                             op, comm, k, sched);
        } else {
            mpi_errno = multiple_buffer_without_dtcopy_recexch(sendbuf, recvbuf, count, datatype,
                                                               op, comm, k, sched);
        }
        goto fn_exit;
    }

    is_inplace = (sendbuf == MPI_IN_PLACE);
    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    tmp_buf = MPIR_TSP_sched_malloc(count * extent, sched);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);

    MPII_Recexchalgo_t recexch;
    MPII_Recexchalgo_start(rank, nranks, k, &recexch);

    reduce_id = (int *) MPL_malloc(sizeof(int) * k, MPL_MEM_COLL);      /* to store reduce vertex ids */
    recv_id = (int *) MPL_malloc(sizeof(int) * recexch.step2_nphases * (k - 1), MPL_MEM_COLL);  /* to store receive vertex ids */
    vtcs = MPL_malloc(sizeof(int) * (recexch.step2_nphases) * k, MPL_MEM_COLL); /* to store graph dependencies */
    MPIR_Assert(reduce_id != NULL && recv_id != NULL && vtcs != NULL);

    if (recexch.in_step2 && !is_inplace && count > 0) { /* copy the data to recvbuf but only if you are a rank participating in Step 2 */
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype,
                                             recvbuf, count, datatype, sched, 0, NULL, &dtcopy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    /* Step 1 */
    MPIR_TSP_Iallreduce_sched_intra_recexch_step1(sendbuf, recvbuf, count,
                                                  datatype, op, is_commutative,
                                                  tag, extent, dtcopy_id, recv_id, reduce_id, vtcs,
                                                  is_inplace, recexch.step1_sendto,
                                                  recexch.in_step2, recexch.step1_nrecvs,
                                                  recexch.step1_recvfrom, per_nbr_buffer,
                                                  &recexch.step1_recvbuf, comm, sched);

    mpi_errno = MPIR_TSP_sched_sink(sched, &step1_id);  /* sink for all the tasks up to end of Step 1 */
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* Step 2 */
    /* allocate memory for receive buffers */
    nbr_buffer =
        (void **) MPL_malloc(sizeof(void *) * recexch.step2_nphases * (k - 1), MPL_MEM_COLL);
    MPIR_Assert(nbr_buffer != NULL);
    buf = 0;
    if (recexch.step2_nphases > 0)
        nbr_buffer[0] = MPIR_TSP_sched_malloc(count * extent, sched);
    for (i = 0; i < recexch.step2_nphases; i++) {
        for (j = 0; j < (k - 1); j++, buf++) {
            if (per_nbr_buffer)
                nbr_buffer[buf] = MPIR_TSP_sched_malloc(count * extent, sched);
            else
                nbr_buffer[buf] = nbr_buffer[0];
        }
    }

    buf = 0;
    for (phase = 0; phase < recexch.step2_nphases && recexch.step1_sendto == -1; phase++) {
        if (!is_commutative) {
            /* sort the neighbor list so that receives can be posted in order */
            MPL_sort_int_array(recexch.step2_nbrs[phase], k - 1);
        }
        /* copy the data to a temporary buffer so that sends ands recvs
         * can be posted simultaneously */
        if (count != 0) {
            if (phase == 0) {   /* wait for Step 1 to complete */
                nvtcs = 1;
                vtcs[0] = step1_id;
            } else {
                /* wait for the previous sends to complete */
                vtcs[0] = imcast_id;
                nvtcs = 1;
                if (is_commutative && per_nbr_buffer == 1) {
                    /* wait for all the previous reductions to complete */
                    MPIR_Localcopy(reduce_id, k - 1, MPI_INT, vtcs + nvtcs, k - 1, MPI_INT);
                    nvtcs += k - 1;
                } else {        /* if it is not commutative or if there is only one recv buffer */
                    /* just wait for the last reduce to complete as reductions happen in sequential order */
                    vtcs[nvtcs++] = reduce_id[k - 2];
                }
            }
            mpi_errno =
                MPIR_TSP_sched_localcopy(recvbuf, count, datatype, tmp_buf, count, datatype, sched,
                                         nvtcs, vtcs, &dtcopy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        }
        /* myidx is the index in the neighbors list such that
         * all neighbors before myidx have ranks less than my rank
         * and neighbors at and after myidx have rank greater than me.
         * This is useful only in the non-commutative case. */
        myidx = 0;
        /* send data to all the neighbors */
        if (count == 0) {
            if (phase == 0) {
                nvtcs = 1;
                vtcs[0] = step1_id;
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "count and phase are 0"));
            } else {    /* wait for all the previous receives to have completed */
                MPIR_Localcopy(recv_id, phase * (k - 1), MPI_INT, vtcs, phase * (k - 1), MPI_INT);
                nvtcs = phase * (k - 1);
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "count 0. Depend on all previous %d recvs.",
                                 nvtcs));
            }
        } else {
            nvtcs = 1;
            vtcs[0] = dtcopy_id;
        }

        if (!is_commutative) {
            for (i = 0; i < k - 1; i++) {
                nbr = recexch.step2_nbrs[phase][i];
                if (rank > nbr) {
                    myidx = i + 1;
                }
            }
        }
        mpi_errno =
            MPIR_TSP_sched_imcast(tmp_buf, count, datatype, recexch.step2_nbrs[phase], k - 1, tag,
                                  comm, sched, nvtcs, vtcs, &imcast_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST, "sending imcast to %d brs. I'm dependent on nvtcs %d",
                         k - 1, nvtcs));

        /* receive and reduce data from the neighbors to the left */
        for (i = myidx - 1, counter = 0; i >= 0; i--, counter++, buf++) {
            nbr = recexch.step2_nbrs[phase][i];
            nvtcs = 0;
            if (count != 0 && per_nbr_buffer == 0 && (phase != 0 || counter != 0)) {
                vtcs[nvtcs++] = (counter == 0) ? reduce_id[k - 2] : reduce_id[counter - 1];
            }
            mpi_errno =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs, &recv_id[buf]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[buf];
                if (counter == 0 || is_commutative) {
                    vtcs[nvtcs++] = dtcopy_id;
                } else {
                    vtcs[nvtcs++] = reduce_id[counter - 1];
                }
                mpi_errno =
                    MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                                sched, nvtcs, vtcs, &reduce_id[counter]);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            }

        }

        /* receive and reduce data from the neighbors to the right */
        for (i = myidx; i < k - 1; i++, counter++, buf++) {
            nbr = recexch.step2_nbrs[phase][i];
            nvtcs = 0;
            if (count != 0 && per_nbr_buffer == 0 && (phase != 0 || counter != 0)) {
                vtcs[nvtcs++] = (counter == 0) ? reduce_id[k - 2] : reduce_id[counter - 1];
            }
            mpi_errno =
                MPIR_TSP_sched_irecv(nbr_buffer[buf], count, datatype, nbr, tag, comm, sched, nvtcs,
                                     vtcs, &recv_id[buf]);

            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

            if (count != 0) {
                nvtcs = 1;
                vtcs[0] = recv_id[buf];
                if (counter == 0 || is_commutative) {
                    vtcs[nvtcs++] = dtcopy_id;
                } else {
                    vtcs[nvtcs++] = reduce_id[counter - 1];
                }
                if (is_commutative) {
                    mpi_errno =
                        MPIR_TSP_sched_reduce_local(nbr_buffer[buf], recvbuf, count, datatype, op,
                                                    sched, nvtcs, vtcs, &reduce_id[counter]);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
                } else {
                    mpi_errno =
                        MPIR_TSP_sched_reduce_local(recvbuf, nbr_buffer[buf], count, datatype, op,
                                                    sched, nvtcs, vtcs, &reduce_id[counter]);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
                    mpi_errno =
                        MPIR_TSP_sched_localcopy(nbr_buffer[buf], count, datatype, recvbuf, count,
                                                 datatype, sched, 1, &reduce_id[counter], &vtx_id);
                    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
                    reduce_id[counter] = vtx_id;
                }
            }
        }
    }

    /* Step 3: This is reverse of Step 1. Ranks that participated in Step 2
     * send the data to non-partcipating ranks */
    if (recexch.step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        mpi_errno =
            MPIR_TSP_sched_irecv(recvbuf, count, datatype, recexch.step1_sendto, tag, comm, sched,
                                 0, NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    } else {
        for (i = 0; i < recexch.step1_nrecvs; i++) {
            if (count == 0) {
                nvtcs = recexch.step2_nphases * (k - 1);
                MPIR_Localcopy(recv_id, recexch.step2_nphases * (k - 1), MPI_INT, vtcs,
                               recexch.step2_nphases * (k - 1), MPI_INT);
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
            mpi_errno =
                MPIR_TSP_sched_isend(recvbuf, count, datatype, recexch.step1_recvfrom[i], tag, comm,
                                     sched, nvtcs, vtcs, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    /* free all allocated memory for storing nbrs */
    MPII_Recexchalgo_finish(&recexch);
    MPL_free(reduce_id);
    MPL_free(recv_id);
    MPL_free(vtcs);
    MPL_free(nbr_buffer);
    if (recexch.in_step2)
        MPL_free(step1_recvbuf);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
