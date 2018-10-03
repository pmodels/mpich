/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*---------------------------------------------------------------------*/
/*     (C) Copyright 2017 Parallel Processing Research Laboratory      */
/*                   Queen's University at Kingston                    */
/*                Neighborhood Collective Communication                */
/*                    Seyed Hessamedin Mirsadeghi                      */
/*---------------------------------------------------------------------*/

#include "mpiimpl.h"

static MPI_Aint find_incom_tmp_buf_size(int **incom_sched_mat, int num_rows, MPI_Aint size_per_rank)
{
    int i;
    MPI_Aint buff_size;
    buff_size = 0;
    for (i = 0; i < num_rows; i++) {
        if (incom_sched_mat[i][1] != -1) {
            /* Initially-ON incoming neighbors */
            buff_size += incom_sched_mat[i][2] * size_per_rank;
        }
    }
    return buff_size;
}

static MPI_Aint find_incom_tmp_buf_offset(int **incom_sched_mat, int nbr_index,
                                          MPI_Aint size_per_rank)
{
    int i;
    MPI_Aint offset;
    offset = 0;
    for (i = 0; i < nbr_index; i++) {
        if (incom_sched_mat[i][1] != -1) {
            /* Initially-ON incoming neighbors */
            offset += incom_sched_mat[i][2] * size_per_rank;
        }
    }
    return offset;
}

/*
 * Message combining
 *
 * Optimized schedule based on common neighborhoods and message combining.
 */

int MPIR_Ineighbor_allgather_intra_sched_comb(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              int recvcount, MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    /* At this point, we already have extracted all information
     * needed for building the schedule. The information is
     * attached to the topo.dist_graph field of the communicator
     * as an nbr_coll_patt structure. Now, we have to build
     * the schedule out of the information provided by this struct.
     */
    /* TODO: Update this implementation based on the new one used for alltoallv. */

    double sched_time = -MPI_Wtime();

    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int i, j;
    int *srcs, *dsts;
    MPIR_CHKLMEM_DECL(2);
    MPIR_SCHED_CHKPMEM_DECL(2);

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, int *, indegree * sizeof(int), mpi_errno, "srcs", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(dsts, int *, outdegree * sizeof(int), mpi_errno, "dsts", MPL_MEM_COMM);
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    Common_nbrhood_matrix *cmn_nbh_mat = topo_ptr->topo.dist_graph.nbh_coll_patt->cmn_nbh_mat;
    int **incom_sched_mat = topo_ptr->topo.dist_graph.nbh_coll_patt->incom_sched_mat;

    MPI_Aint recvtype_extent, recvtype_true_extent, recvtype_max_extent, recvtype_true_lb;
    MPI_Aint sendtype_extent, sendtype_true_extent, sendtype_max_extent, sendtype_true_lb;
    MPI_Aint recvbuf_extent;
    MPI_Aint sendbuf_extent;
    MPI_Aint exchange_tmp_buf_extent;
    MPI_Aint incom_tmp_buf_extent;
    MPI_Aint exchange_recv_count, exchange_send_count;
    MPI_Aint incom_recv_count;

    void *exchange_tmp_buf = NULL;
    void *incom_tmp_buf = NULL;

    /* get extent and true extent of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &sendtype_true_extent);
    recvtype_max_extent = MPL_MAX(recvtype_true_extent, recvtype_extent);
    sendtype_max_extent = MPL_MAX(sendtype_true_extent, sendtype_extent);

    /* get extent (true) of sendbuf and recvbuf */
    recvbuf_extent = recvcount * recvtype_max_extent;
    sendbuf_extent = sendcount * sendtype_max_extent;

    /* TODO: What is the largest offset we add to buffers?
     * MPIR_Ensure_Aint_fits_in_pointer()?
     */

    exchange_tmp_buf_extent = 2 * sendbuf_extent;       /* locally known for a simple allgather */
    incom_tmp_buf_extent = find_incom_tmp_buf_size(incom_sched_mat, indegree, recvbuf_extent);
    MPIR_SCHED_CHKPMEM_MALLOC(exchange_tmp_buf, void *, exchange_tmp_buf_extent,
                              mpi_errno, "exchange_tmp_buf", MPL_MEM_OTHER);
    MPIR_SCHED_CHKPMEM_MALLOC(incom_tmp_buf, void *, incom_tmp_buf_extent,
                              mpi_errno, "incom_tmp_buf", MPL_MEM_OTHER);

    int t;
    for (t = 0; t < cmn_nbh_mat->t; t++) {
        /** Building the schedule one time step at a time **/

        /* Compiling cmn_nbh_mat first */
        for (i = 0; i < cmn_nbh_mat->num_rows; i++) {
            if (cmn_nbh_mat->comb_matrix[i][t].opt != COMB_IDLE) {
                /* We have to exchange with a friend */
                exchange_recv_count = sendcount;
                exchange_send_count = sendcount;
                char *rb = ((char *) exchange_tmp_buf) + exchange_send_count * sendtype_extent;
                mpi_errno = MPIR_Sched_recv(rb, exchange_recv_count, sendtype,
                                            cmn_nbh_mat->comb_matrix[i][t].paired_frnd,
                                            comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

                mpi_errno = MPIR_Sched_send(sendbuf, exchange_send_count, sendtype,
                                            cmn_nbh_mat->comb_matrix[i][t].paired_frnd,
                                            comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

                /* Schedule a copy from sendbuf to exchange_tmp_buf */
                char *copy_from = ((char *) sendbuf);
                char *copy_to = ((char *) exchange_tmp_buf);
                MPIR_Sched_copy(copy_from, sendcount, sendtype, copy_to, sendcount, sendtype, s);

                MPIR_SCHED_BARRIER(s);

                /* Schedule to send the combined message to corresponding neighbors */
                int k;
                for (k = i; k < cmn_nbh_mat->num_rows; k++) {
                    if (!cmn_nbh_mat->is_row_offloaded[k] &&
                        cmn_nbh_mat->comb_matrix[k][t].opt != COMB_IDLE) {
                        mpi_errno = MPIR_Sched_send(exchange_tmp_buf,
                                                    exchange_send_count + exchange_recv_count,
                                                    sendtype, dsts[k], comm_ptr, s);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                    }
                }

                break;  /* we have a maximum of one paired friend at each time step t */
            }
        }

        /* Compiling the incom_sched_mat */
        /* Scheduling the necessary recv operations */
        for (i = 0; i < indegree; i++) {
            if (!incom_sched_mat[i][0]) {
                /* On incoming neighbors */
                if (incom_sched_mat[i][1] == t) {
                    /* Schedule a receive from the corresponding source */
                    incom_recv_count = incom_sched_mat[i][2] * recvcount;
                    char *rb = ((char *) incom_tmp_buf) +
                        find_incom_tmp_buf_offset(incom_sched_mat, i, recvbuf_extent);
                    mpi_errno = MPIR_Sched_recv(rb, incom_recv_count, recvtype,
                                                incom_sched_mat[i][COMB_LIST_START_IDX],
                                                comm_ptr, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
            } else {    /* Off incoming neighbors */

                /* Nothing to do with no cumulative */
            }
        }

        MPIR_SCHED_BARRIER(s);

        /* Scheduling message copies from incom_tmp_buf to final recv_buf */
        for (i = 0; i < indegree; i++) {
            if (!incom_sched_mat[i][0]) {
                /* On incoming neighbors */
                if (incom_sched_mat[i][1] == t) {
                    MPI_Aint offset1 = find_incom_tmp_buf_offset(incom_sched_mat,
                                                                 i, recvbuf_extent);
                    for (j = 0; j < incom_sched_mat[i][2]; j++) {
                        /* Each rank in combining list */
                        int nbr_idx = find_in_arr(srcs, indegree,
                                                  incom_sched_mat[i][j + COMB_LIST_START_IDX]);
                        char *copy_from = ((char *) incom_tmp_buf) + offset1 +
                            j * recvcount * recvtype_extent;
                        char *copy_to = ((char *) recvbuf) + nbr_idx * recvcount * recvtype_extent;
                        MPIR_Sched_copy(copy_from, recvcount, recvtype,
                                        copy_to, recvcount, recvtype, s);
                    }
                }
            }
        }

        MPIR_SCHED_BARRIER(s);
    }

    MPIR_SCHED_BARRIER(s);

    /* Schedule a send for any residual outgoing neighbors */
    for (i = 0; i < cmn_nbh_mat->num_rows; i++) {
        if (!cmn_nbh_mat->ignore_row[i]) {
            /* out neighbor still active */
            mpi_errno = MPIR_Sched_send(sendbuf, sendcount, sendtype, dsts[i], comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    /* Schedule a recv for any residual incoming neighbors */
    /* NOTE: the incoming message can still be a combined one.
     * TODO: We might be able to use 't' to schedule these
     * residual recvs in a stepwise fashion to create some
     * overlap among the recv and copy operations.
     */
    for (i = 0; i < indegree; i++) {
        if (!incom_sched_mat[i][0] && incom_sched_mat[i][1] >= cmn_nbh_mat->t) {
            /* On incoming neighbors not covered before */
            /* Schedule a receive from the corresponding source */
            incom_recv_count = incom_sched_mat[i][2] * recvcount;
            char *rb = ((char *) incom_tmp_buf) +
                find_incom_tmp_buf_offset(incom_sched_mat, i, recvbuf_extent);
            mpi_errno = MPIR_Sched_recv(rb, incom_recv_count, recvtype,
                                        incom_sched_mat[i][COMB_LIST_START_IDX], comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        } else {        /* Off incoming neighbors not covered before */

            /* Nothing to do with no cumulative */
        }
    }

    MPIR_SCHED_BARRIER(s);

    /* Scheduling message copies from incom_tmp_buf to final recv_buf */
    for (i = 0; i < indegree; i++) {
        if (!incom_sched_mat[i][0] && incom_sched_mat[i][1] >= cmn_nbh_mat->t) {
            /* On incoming neighbors not covered before */
            MPI_Aint offset1 = find_incom_tmp_buf_offset(incom_sched_mat, i, recvbuf_extent);
            for (j = 0; j < incom_sched_mat[i][2]; j++) {
                int nbr_idx = find_in_arr(srcs, indegree,
                                          incom_sched_mat[i][j + COMB_LIST_START_IDX]);
                char *copy_from = ((char *) incom_tmp_buf) + offset1 +
                    j * recvcount * recvtype_extent;
                char *copy_to = ((char *) recvbuf) + nbr_idx * recvcount * recvtype_extent;
                MPIR_Sched_copy(copy_from, recvcount, recvtype, copy_to, recvcount, recvtype, s);
            }
        }
    }

    MPIR_SCHED_BARRIER(s);

    sched_time += MPI_Wtime();

    MPIR_SCHED_CHKPMEM_COMMIT(s);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
