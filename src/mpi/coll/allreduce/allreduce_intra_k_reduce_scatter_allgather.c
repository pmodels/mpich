/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "recexchalgo.h"
#include "algo_common.h"


int MPIR_Allreduce_intra_k_reduce_scatter_allgather(const void *sendbuf,
                                                    void *recvbuf,
                                                    MPI_Aint count,
                                                    MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm, int k,
                                                    int single_phase_recv, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int rank, nranks, nbr;
    int rem = 0, idx = 0, dst = 0, rank_for_offset;
    MPI_Aint true_extent, true_lb, extent;
    int current_cnt = 0, send_count = 0, recv_count = 0, send_cnt = 0, recv_cnt = 0, iter = 0;
    int offset = 0;
    int step1_sendto = -1, step1_nrecvs = 0, *step1_recvfrom = NULL;
    int step2_nphases = 0, **step2_nbrs;
    int i, j, x, phase, recv_phase, p_of_k, T;
    bool in_step2 = true;
    int index = 0, comm_alloc = 1;
    MPIR_Request *sreqs[MAX_RADIX * 2], *rreqs[MAX_RADIX * 2];
    MPIR_Request **send_reqs = NULL, **recv_reqs = NULL;
    int num_sreq = 0, num_rreq = 0, total_phases = 0;
    void *tmp_recvbuf = NULL;
    MPIR_CHKLMEM_DECL(2);

    MPIR_Assert(k > 1);

    rank = comm->rank;
    nranks = comm->local_size;
    MPIR_Assert(MPIR_Op_is_commutative(op));

    /* need to allocate temporary buffer to store incoming data */
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE && count > 0) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

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
        index = k - 2;  /* we calculate starting k = 2 */
        /* If this is the first time a recursive exchange is called with k, populate the nbr calculation in
         * the communicator */
        if (comm->coll.nbrs_defined[index] == 0) {
            comm->coll.nbrs_defined[index] = 1;
            comm->coll.k[index] = k;
            comm->coll.step1_sendto[index] = -1;
            comm->coll.step1_nrecvs[index] = 0;
            comm->coll.step2_nphases[index] = 0;
            mpi_errno = MPII_Recexchalgo_get_neighbors(rank, nranks, &comm->coll.k[index],
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
    /* allocate temp buffer for receiving in reduce scatter phase */
    tmp_recvbuf = MPL_malloc(count * extent, MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(!tmp_recvbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");

    in_step2 = (step1_sendto == -1);    /* whether this rank participates in Step 2 */
    if (!in_step2) {    /* even */
        /* non-participating rank sends the data to a participating rank */
        mpi_errno = MPIC_Send(recvbuf, count,
                              datatype, step1_sendto, MPIR_ALLREDUCE_TAG, comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

    } else {    /* odd */
        for (i = 0; i < step1_nrecvs; i++) {    /* participating rank gets data from non-partcipating ranks */
            mpi_errno =
                MPIC_Recv(tmp_recvbuf, count, datatype, step1_recvfrom[i], MPIR_ALLREDUCE_TAG, comm,
                          MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            /* Do reduction of reduced data */
            mpi_errno = MPIR_Reduce_local(tmp_recvbuf, recvbuf, count, datatype, op);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

    /* Main recursive exchange step */
    if (in_step2) {
        MPI_Aint *cnts = NULL, *displs = NULL;
        MPIR_CHKLMEM_MALLOC(cnts, MPI_Aint *, sizeof(MPI_Aint) * nranks, mpi_errno, "cnts",
                            MPL_MEM_COLL);
        MPIR_CHKLMEM_MALLOC(displs, MPI_Aint *, sizeof(MPI_Aint) * nranks, mpi_errno, "displs",
                            MPL_MEM_COLL);
        idx = 0;
        rem = nranks - p_of_k;

        /* create a counts and displacements array for reduce scatter and allgatherv stages */
        for (i = 0; i < nranks; i++)
            cnts[i] = 0;
        for (i = 0; i < (p_of_k - 1); i++) {
            idx = (i < rem / (k - 1)) ? (i * k) + (k - 1) : i + rem;
            cnts[idx] = count / p_of_k;
        }
        idx = (p_of_k - 1 < rem / (k - 1)) ? (p_of_k - 1 * k) + (k - 1) : p_of_k - 1 + rem;
        cnts[idx] = count - (count / p_of_k) * (p_of_k - 1);

        displs[0] = 0;
        for (i = 1; i < nranks; i++) {
            displs[i] = displs[i - 1] + cnts[i - 1];
        }

        /* reduce scatter - distance doubling and vector halving. Exchange data with the closest nbr in the first phase
         * and increase the distance between nbrs in subsequent phases while the data decreases */
        for (phase = 0, j = step2_nphases - 1; phase < step2_nphases; j--, phase++) {
            /* send data to all the neighbors */
            for (i = 0; i < k - 1; i++) {
                num_rreq = 0;
                dst = step2_nbrs[phase][i];

                rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(dst, nranks, k);
                MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks,
                                                      &current_cnt, &offset);
                MPI_Aint send_offset = displs[offset] * extent;
                send_cnt = 0;
                for (x = 0; x < current_cnt; x++)
                    send_cnt += cnts[offset + x];
                mpi_errno =
                    MPIC_Isend((char *) recvbuf + send_offset, send_cnt,
                               datatype, dst, MPIR_ALLREDUCE_TAG, comm, &recv_reqs[num_rreq++],
                               errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
                MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks,
                                                      &current_cnt, &offset);

                MPI_Aint recv_offset = displs[offset] * extent;
                recv_cnt = 0;
                for (x = 0; x < current_cnt; x++)
                    recv_cnt += cnts[offset + x];
                mpi_errno =
                    MPIC_Irecv((char *) tmp_recvbuf + recv_offset, recv_cnt, datatype,
                               dst, MPIR_ALLREDUCE_TAG, comm, &recv_reqs[num_rreq++]);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                mpi_errno = MPIC_Waitall(num_rreq, recv_reqs, MPI_STATUSES_IGNORE, errflag);
                if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                    MPIR_ERR_POP(mpi_errno);
                mpi_errno =
                    MPIR_Reduce_local((char *) tmp_recvbuf + recv_offset,
                                      (char *) recvbuf + recv_offset, recv_cnt, datatype, op);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }

        /* allgatherv - distance halving and vector doubling. Exchange data with the farthest nbr in the first phase
         * and decrease the distance between nbrs in subsequent phases while the data increases */
        phase = step2_nphases - 1;
        recv_phase = step2_nphases - 1;
        for (j = 0; j < step2_nphases && step1_sendto == -1; j++) {
            /* post recvs for two phases. If optimization enabled, post for one phase  */
            total_phases = (single_phase_recv) ? 1 : 2;
            num_sreq = 0;
            num_rreq = 0;
            for (iter = 0; iter < total_phases && (j + iter) < step2_nphases; iter++) {
                for (i = 0; i < k - 1; i++) {
                    nbr = step2_nbrs[recv_phase][i];
                    rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(nbr, nranks, k);

                    MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j + iter, k, nranks,
                                                          &current_cnt, &offset);
                    MPI_Aint recv_offset = displs[offset] * extent;
                    recv_count = 0;
                    for (x = 0; x < current_cnt; x++)
                        recv_count += cnts[offset + x];
                    mpi_errno = MPIC_Irecv(((char *) recvbuf + recv_offset), recv_count, datatype,
                                           nbr, MPIR_ALLREDUCE_TAG, comm, &recv_reqs[num_rreq++]);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag =
                            MPIX_ERR_PROC_FAILED ==
                            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                recv_phase--;
            }
            /* send data to all the neighbors */
            for (i = 0; i < k - 1; i++) {
                nbr = step2_nbrs[phase][i];
                rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
                MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks, &current_cnt,
                                                      &offset);
                MPI_Aint send_offset = displs[offset] * extent;
                send_count = 0;
                for (x = 0; x < current_cnt; x++)
                    send_count += cnts[offset + x];
                mpi_errno = MPIC_Isend(((char *) recvbuf + send_offset), send_count, datatype,
                                       nbr, MPIR_ALLREDUCE_TAG, comm, &send_reqs[num_sreq++],
                                       errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            /* wait on prev recvs */
            mpi_errno = MPIC_Waitall((k - 1), recv_reqs, MPI_STATUSES_IGNORE, errflag);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIR_ERR_POP(mpi_errno);

            phase--;
            if (single_phase_recv == false) {
                j++;
                if (j < step2_nphases) {
                    /* send data to all the neighbors */
                    for (i = 0; i < k - 1; i++) {
                        nbr = step2_nbrs[phase][i];
                        rank_for_offset = MPII_Recexchalgo_reverse_digits_step2(rank, nranks, k);
                        MPII_Recexchalgo_get_count_and_offset(rank_for_offset, j, k, nranks,
                                                              &current_cnt, &offset);
                        MPI_Aint send_offset = displs[offset] * extent;
                        send_count = 0;
                        for (x = 0; x < current_cnt; x++)
                            send_count += cnts[offset + x];
                        mpi_errno =
                            MPIC_Isend(((char *) recvbuf + send_offset), send_count, datatype, nbr,
                                       MPIR_ALLREDUCE_TAG, comm, &send_reqs[num_sreq++], errflag);
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag =
                                MPIX_ERR_PROC_FAILED ==
                                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED :
                                MPIR_ERR_OTHER;
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
                    }
                    /* wait on prev recvs */
                    mpi_errno =
                        MPIC_Waitall((k - 1), recv_reqs + (k - 1), MPI_STATUSES_IGNORE, errflag);
                    if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                        MPIR_ERR_POP(mpi_errno);

                    phase--;
                }
            }
            mpi_errno = MPIC_Waitall(num_sreq, send_reqs, MPI_STATUSES_IGNORE, errflag);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    /* Step 3: This is reverse of Step 1. Rans that participated in Step 2
     * send the data to non-partcipating rans */
    if (step1_sendto != -1) {   /* I am a Step 2 non-participating rank */
        mpi_errno = MPIC_Recv(recvbuf, count, datatype, step1_sendto, MPIR_ALLREDUCE_TAG, comm,
                              MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    } else {
        if (step1_nrecvs > 0) {
            for (i = 0; i < step1_nrecvs; i++) {
                mpi_errno =
                    MPIC_Isend(recvbuf, count, datatype, step1_recvfrom[i], MPIR_ALLREDUCE_TAG,
                               comm, &send_reqs[i], errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            mpi_errno = MPIC_Waitall(i, send_reqs, MPI_STATUSES_IGNORE, errflag);
            if (mpi_errno && mpi_errno != MPI_ERR_IN_STATUS)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    if (!comm_alloc) {
        /* free the memory */
        for (i = 0; i < step2_nphases; i++)
            MPL_free(step2_nbrs[i]);
        MPL_free(step2_nbrs);
        MPL_free(step1_recvfrom);
        if (in_step2) {
            MPL_free(recv_reqs);
            MPL_free(send_reqs);
        }
    }
    MPL_free(tmp_recvbuf);
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
