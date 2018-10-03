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

#ifdef SCHED_DEBUG
int debug_glob_all_reqs_idx = 0;
MPI_Request debug_glob_all_reqs[MAX_DEBUG_SCHED_REQS];
#endif

static int find_array_sum(const int *array, int size)
{
    int sum = 0;
    int i;
    for (i = 0; i < size; i++) {
        sum += array[i];
    }
    return sum;
}

static int a2av_make_all_combined_msgs(int t, void *exchange_recvbuf,
                                       int *friend_off_counts, const void *sendbuf,
                                       const int *sendcounts, const int *sdispls, const int *dests,
                                       MPI_Datatype sendtype, MPI_Aint sendtype_extent,
                                       MPI_Aint sendtype_max_extent,
                                       Common_nbrhood_matrix * cmn_nbh_mat, MPIR_Comm * comm_ptr,
                                       MPIR_Sched_t s, void **combined_sendbuf_ptr)
{
#ifdef SCHED_DEBUG
    char content[1024];
#endif
    MPIR_SCHED_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    int i, dest_idx, f_displ = 0, fc_idx = 0;
    int onload_start = cmn_nbh_mat->sorted_cmn_nbrs[t].onload_start;
    int onload_end = cmn_nbh_mat->sorted_cmn_nbrs[t].onload_end;
    /* allocate combined_sendbuf */
    int combined_sendbuf_count = 0;
    MPI_Aint combined_sendbuf_extent = 0;
    for (i = onload_start; i <= onload_end; i++) {
        dest_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[i].index;
        combined_sendbuf_count += sendcounts[dest_idx] + friend_off_counts[fc_idx];
        fc_idx++;
    }

#ifdef SCHED_DEBUG
    sprintf(content,
            "combined_send_count = %d (for all onloaded at t = %d)\n", combined_sendbuf_count, t);
    print_in_file(comm_ptr->rank, content);
#endif

    combined_sendbuf_extent = combined_sendbuf_count * sendtype_max_extent;

    void *combined_sendbuf = NULL;
    MPIR_SCHED_CHKPMEM_MALLOC(combined_sendbuf, void *, combined_sendbuf_extent,
                              mpi_errno, "combined_sendbuf", MPL_MEM_OTHER);
    *(char **) combined_sendbuf_ptr = combined_sendbuf; /* to return to the caller */

#ifdef SCHED_DEBUG
    int num_onloaded = cmn_nbh_mat->num_onloaded[t];
    print_vec(comm_ptr->rank, find_array_sum(friend_off_counts, num_onloaded),
              exchange_recvbuf, "exchange_recvbuf in make_all_combined: ");
#endif

    /**** pack the combined message from sendbuf and exchange_recvbuf into combined_buf ****/
    char *copy_from;
    char *copy_to = ((char *) combined_sendbuf);
    f_displ = fc_idx = 0;
    for (i = onload_start; i <= onload_end; i++) {
        dest_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[i].index;
        /* copy own data first */
        copy_from = ((char *) sendbuf) + sdispls[dest_idx] * sendtype_extent;
        MPIR_Sched_copy(copy_from, sendcounts[dest_idx], sendtype,
                        copy_to, sendcounts[dest_idx], sendtype, s);

#ifdef SCHED_DEBUG
        MPIR_Localcopy(copy_from, sendcounts[dest_idx], sendtype,
                       copy_to, sendcounts[dest_idx], sendtype);
#endif

        copy_to += sendcounts[dest_idx] * sendtype_extent;
        /* append friend's data */
        copy_from = ((char *) exchange_recvbuf) + f_displ * sendtype_extent;
        MPIR_Sched_copy(copy_from, friend_off_counts[fc_idx], sendtype,
                        copy_to, friend_off_counts[fc_idx], sendtype, s);

#ifdef SCHED_DEBUG
        MPIR_Localcopy(copy_from, friend_off_counts[fc_idx], sendtype,
                       copy_to, friend_off_counts[fc_idx], sendtype);
#endif

        copy_to += friend_off_counts[fc_idx] * sendtype_extent;
        f_displ += friend_off_counts[fc_idx];   /* increase displacement in exchange_recvbuf
                                                 * for next iteration */
        fc_idx++;
        /**** End of packing ****/
    }

#ifdef SCHED_DEBUG
    print_vec(comm_ptr->rank, combined_sendbuf_count,
              (int *) combined_sendbuf, "combined_sendbuf in make_all_combined: ");
#endif

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

static int a2av_send_to_onloaded_nbrs(int t, void *combined_sendbuf, int *friend_off_counts,
                                      const int *sendcounts, const int *sdispls, const int *dests,
                                      MPI_Datatype sendtype, MPI_Aint sendtype_extent,
                                      Common_nbrhood_matrix * cmn_nbh_mat, MPIR_Comm * comm_ptr,
                                      MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, dest_idx, fc_idx = 0;
    int onload_start = cmn_nbh_mat->sorted_cmn_nbrs[t].onload_start;
    int onload_end = cmn_nbh_mat->sorted_cmn_nbrs[t].onload_end;

    char *send_from = (char *) combined_sendbuf;
    int combined_sendcount;
    fc_idx = 0;
    for (i = onload_start; i <= onload_end; i++) {
        dest_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[i].index;
        combined_sendcount = sendcounts[dest_idx] + friend_off_counts[fc_idx];
        mpi_errno = MPIR_Sched_send(send_from, combined_sendcount,
                                    sendtype, dests[dest_idx], comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

#ifdef SCHED_DEBUG
        int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
            MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
        MPIR_Request *req_ptr = NULL;
        mpi_errno = MPID_Isend(send_from, combined_sendcount,
                               sendtype, dests[dest_idx], 1000, comm_ptr, context_offset, &req_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        debug_glob_all_reqs[debug_glob_all_reqs_idx++] = req_ptr->handle;

        char tmp_string[256];
        sprintf(tmp_string, "buffer sent to %d: ", dests[dest_idx]);
        print_vec(comm_ptr->rank, combined_sendcount, (int *) send_from, tmp_string);
#endif

        send_from += combined_sendcount * sendtype_extent;
        fc_idx++;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int a2av_send_own_off_counts(int t, const int *sendcounts,
                                    Common_nbrhood_matrix * cmn_nbh_mat,
                                    MPIR_Comm * comm_ptr, MPI_Request * mpi_req_ptr,
                                    int **own_off_counts_ptr)
{
    /***** PACK AND SEND OWN_OFF_COUNTS TO FRIEND *****/
    MPIR_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    int i, dest_idx, idx = 0;
    int num_offloaded = cmn_nbh_mat->num_offloaded[t];
    int offload_start = cmn_nbh_mat->sorted_cmn_nbrs[t].offload_start;
    int offload_end = cmn_nbh_mat->sorted_cmn_nbrs[t].offload_end;
    /* find friend rank from any of onloaded/offloaded neighbor entries in comb_matrix (at t) */
    int tmp_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[offload_start].index;
    int friend = cmn_nbh_mat->comb_matrix[tmp_idx][t].paired_frnd;
    /* allocate own ranks_counts. size is 2*num_offloaded integers */
    int *own_off_counts;
    MPIR_CHKPMEM_MALLOC(own_off_counts, int *, num_offloaded * sizeof(int),
                        mpi_errno, "own_off_counts", MPL_MEM_OTHER);
    *own_off_counts_ptr = own_off_counts;       /* to return to the caller */
    /* populate it from dsts and sendcounts arrays */
    for (i = offload_start; i <= offload_end; i++) {
        dest_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[i].index;
        own_off_counts[idx++] = sendcounts[dest_idx];
    }
    /* send own_off_counts to the friend */
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
    MPIR_Request *req_ptr = NULL;
    mpi_errno = MPID_Isend(own_off_counts, num_offloaded, MPI_INT, friend,
                           1000, comm_ptr, context_offset, &req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    *mpi_req_ptr = req_ptr->handle;

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

static int a2av_get_friend_off_counts(int t, Common_nbrhood_matrix * cmn_nbh_mat,
                                      MPIR_Comm * comm_ptr, MPI_Request * mpi_req_ptr,
                                      int **friend_off_counts_ptr)
{
    /***** GET friend_off_counts *****/
    MPIR_CHKPMEM_DECL(1);
    int mpi_errno = MPI_SUCCESS;
    int num_onloaded = cmn_nbh_mat->num_onloaded[t];
    int onload_start = cmn_nbh_mat->sorted_cmn_nbrs[t].onload_start;
    /* find friend rank from any of onloaded/offloaded neighbor entries in comb_matrix (at t) */
    int tmp_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[onload_start].index;
    int friend = cmn_nbh_mat->comb_matrix[tmp_idx][t].paired_frnd;
    /* allocate ranks+counts. size is 2*num_onloaded integers */
    int *friend_off_counts;
    MPIR_CHKPMEM_MALLOC(friend_off_counts, int *, num_onloaded * sizeof(int),
                        mpi_errno, "friend_off_counts", MPL_MEM_OTHER);
    *friend_off_counts_ptr = friend_off_counts; /* to return to the caller */
    /* recv from friend into friend_off_counts */
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
    MPIR_Request *req_ptr = NULL;
    mpi_errno = MPID_Irecv(friend_off_counts, num_onloaded, MPI_INT, friend,
                           1000, comm_ptr, context_offset, &req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    *mpi_req_ptr = req_ptr->handle;

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

static int a2av_exchange_data_with_friend(int t, const void *sendbuf,
                                          const int *sendcounts, const int *sdispls,
                                          MPI_Datatype sendtype, MPI_Aint sendtype_extent,
                                          MPI_Aint sendtype_max_extent, int *own_off_counts,
                                          int *friend_off_counts,
                                          Common_nbrhood_matrix * cmn_nbh_mat, MPIR_Comm * comm_ptr,
                                          MPIR_Sched_t s, void **exchange_sendbuf_ptr,
                                          void **exchange_recvbuf_ptr)
{
    MPIR_SCHED_CHKPMEM_DECL(2);
    int mpi_errno = MPI_SUCCESS;
    int i, dest_idx;
    int num_offloaded = cmn_nbh_mat->num_offloaded[t];
    int offload_start = cmn_nbh_mat->sorted_cmn_nbrs[t].offload_start;
    int offload_end = cmn_nbh_mat->sorted_cmn_nbrs[t].offload_end;
    int num_onloaded = cmn_nbh_mat->num_onloaded[t];
    /* find friend rank from any of onloaded/offloaded neighbor entries in comb_matrix (at t) */
    int tmp_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[offload_start].index;
    int friend = cmn_nbh_mat->comb_matrix[tmp_idx][t].paired_frnd;
    /* find exchange_sendbuf_extent */
    int own_off_counts_sum = find_array_sum(own_off_counts, num_offloaded);
    MPI_Aint exchange_sendbuf_extent = own_off_counts_sum * sendtype_max_extent;
    /* find exchange_recvbuf_extent */
    int friend_off_counts_sum = find_array_sum(friend_off_counts, num_onloaded);
    MPI_Aint exchange_recvbuf_extent = friend_off_counts_sum * sendtype_max_extent;
    /* allocate exchange_sendbuf and exchange_recvbuf */
    void *exchange_sendbuf = NULL;
    void *exchange_recvbuf = NULL;
    MPIR_SCHED_CHKPMEM_MALLOC(exchange_sendbuf, void *, exchange_sendbuf_extent,
                              mpi_errno, "exchange_sendbuf", MPL_MEM_OTHER);
    MPIR_SCHED_CHKPMEM_MALLOC(exchange_recvbuf, void *, exchange_recvbuf_extent,
                              mpi_errno, "exchange_recvbuf", MPL_MEM_OTHER);
    *(char **) exchange_sendbuf_ptr = exchange_sendbuf; /* to return to the caller */
    *(char **) exchange_recvbuf_ptr = exchange_recvbuf; /* to return to the caller */
    /* populate exchange_send_buf from sendbuf */
    char *copy_from;
    char *copy_to = (char *) exchange_sendbuf;
    for (i = offload_start; i <= offload_end; i++) {
        dest_idx = cmn_nbh_mat->sorted_cmn_nbrs[t].cmn_nbrs[i].index;
        copy_from = ((char *) sendbuf) + sdispls[dest_idx] * sendtype_extent;
        MPIR_Sched_copy(copy_from, sendcounts[dest_idx], sendtype,
                        copy_to, sendcounts[dest_idx], sendtype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

#ifdef SCHED_DEBUG
        MPIR_Localcopy(copy_from, sendcounts[dest_idx], sendtype,
                       copy_to, sendcounts[dest_idx], sendtype);
#endif

        copy_to += sendcounts[dest_idx] * sendtype_extent;
    }

    MPIR_SCHED_BARRIER(s);

#ifdef SCHED_DEBUG
    int all_reqs_idx = 0;
    MPI_Request all_reqs[2];
    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
    MPIR_Request *req_ptr = NULL;
    mpi_errno = MPID_Isend(exchange_sendbuf, own_off_counts_sum, sendtype, friend,
                           1000, comm_ptr, context_offset, &req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    all_reqs[all_reqs_idx++] = req_ptr->handle;

    mpi_errno = MPID_Irecv(exchange_recvbuf, friend_off_counts_sum, sendtype, friend,
                           1000, comm_ptr, context_offset, &req_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    all_reqs[all_reqs_idx++] = req_ptr->handle;

    mpi_errno = MPIR_Waitall(all_reqs_idx, all_reqs, MPI_STATUS_IGNORE);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    all_reqs_idx = 0;   /* set index back to zero for future use */

    print_vec(comm_ptr->rank, own_off_counts_sum, (int *) exchange_sendbuf, "exchange_sendbuf: ");
    print_vec(comm_ptr->rank, friend_off_counts_sum,
              (int *) exchange_recvbuf, "exchange_recvbuf: ");
#endif

    /* send exchange_sendbuf to friend
     * - type is obviously sendtype.
     * - count is own_off_counts_count_sum.
     */
    mpi_errno = MPIR_Sched_send(exchange_sendbuf,
                                own_off_counts_sum, sendtype, friend, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    /* recv into exchange_recvbuf from friend
     * - type is sendtype because the incoming data was
     *   supposed to be sent to neighbors common to this rank.
     * - count is friend_off_counts_count_sum.
     */
    mpi_errno = MPIR_Sched_recv(exchange_recvbuf,
                                friend_off_counts_sum, sendtype, friend, comm_ptr, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}

static int a2av_find_incom_recv_count(int *incom_sched_vec, int *srcs,
                                      const int *recvcounts, int indegree)
{
    int i, nbr_idx, sum = 0;
    for (i = 0; i < incom_sched_vec[2]; i++) {
        nbr_idx = find_in_arr(srcs, indegree, incom_sched_vec[i + COMB_LIST_START_IDX]);
        sum += recvcounts[nbr_idx];
    }
    return sum;
}

/*
 * Message combining
 *
 * Optimized schedule based on common neighborhoods and message combining.
 */

int MPIR_Ineighbor_alltoallv_intra_sched_comb(const void *sendbuf, const int sendcounts[],
                                              const int sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const int recvcounts[],
                                              const int rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    double sched_time = -MPI_Wtime();

    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int i, j;
    int *srcs, *dsts;
    MPIR_CHKLMEM_DECL(2);
    MPIR_SCHED_CHKPMEM_DECL(1);

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

#ifdef DEBUG
    char content[1024];         /* for in-file prints */
#endif

    MPIR_Topology *topo_ptr = MPIR_Topology_get(comm_ptr);
    Common_nbrhood_matrix *cmn_nbh_mat = topo_ptr->topo.dist_graph.nbh_coll_patt->cmn_nbh_mat;
    int **incom_sched_mat = topo_ptr->topo.dist_graph.nbh_coll_patt->incom_sched_mat;

    MPI_Aint recvtype_extent, recvtype_true_extent, recvtype_max_extent, recvtype_true_lb;
    MPI_Aint sendtype_extent, sendtype_true_extent, sendtype_max_extent, sendtype_true_lb;
    MPI_Aint recvbuf_extent;

    /* get extent and true extent of sendtype and recvtype */
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_true_lb, &recvtype_true_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_true_lb, &sendtype_true_extent);
    recvtype_max_extent = MPL_MAX(recvtype_true_extent, recvtype_extent);
    sendtype_max_extent = MPL_MAX(sendtype_true_extent, sendtype_extent);

    /* find the total sum for sendcounts and recvcounts */
    int recvcount_sum = 0;
    recvcount_sum = find_array_sum(recvcounts, indegree);

    /* get extent (true) of sendbuf and recvbuf */
    recvbuf_extent = recvcount_sum * recvtype_max_extent;

    /* TODO: What is the largest offset we add to buffers?
     * MPIR_Ensure_Aint_fits_in_pointer()?
     */

    void *sched_bufs_from_funcs[SCHED_MEM_TO_FREE_MAX_SIZE] = { NULL };
    int sched_bufs_from_funcs_count = 0;

    /* allocate temporary buffer to receive data from incoming neighbors */
    MPI_Aint incom_tmp_buf_extent;
    void *incom_tmp_buf = NULL;
    incom_tmp_buf_extent = recvbuf_extent;
    MPIR_SCHED_CHKPMEM_MALLOC(incom_tmp_buf, void *, incom_tmp_buf_extent,
                              mpi_errno, "incom_tmp_buf", MPL_MEM_OTHER);

    int all_reqs_idx = 0;
    MPI_Request all_reqs[2];

    int t;
    int *own_off_counts = NULL;
    int *friend_off_counts = NULL;
    void *exchange_sendbuf = NULL;
    void *exchange_recvbuf = NULL;
    void *combined_sendbuf = NULL;
    for (t = 0; t < cmn_nbh_mat->t; t++) {
        /** Building the schedule one time step at a time **/

        /* Compiling cmn_nbh_mat first */
        if (cmn_nbh_mat->num_onloaded[t] > 0) {
            /***** GET FRIEND_OFF_COUNTS *****/
            mpi_errno = a2av_get_friend_off_counts(t, cmn_nbh_mat, comm_ptr,
                                                   &(all_reqs[all_reqs_idx++]), &friend_off_counts);

            /***** SEND OWN_OFF_COUNTS TO FRIEND *****/
            mpi_errno = a2av_send_own_off_counts(t, sendcounts, cmn_nbh_mat, comm_ptr,
                                                 &(all_reqs[all_reqs_idx++]), &own_off_counts);

            mpi_errno = MPIR_Waitall(all_reqs_idx, all_reqs, MPI_STATUS_IGNORE);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            all_reqs_idx = 0;   /* set index back to zero for future use */

#ifdef SCHED_DEBUG
            print_vec(comm_ptr->rank,
                      cmn_nbh_mat->num_onloaded[t], friend_off_counts, "friend_off_counts: ");
            print_vec(comm_ptr->rank,
                      cmn_nbh_mat->num_offloaded[t], own_off_counts, "own_off_counts: ");
#endif

            /***** EXCHANGE DATA WITH FRIEND *****/
            mpi_errno = a2av_exchange_data_with_friend(t, sendbuf, sendcounts, sdispls, sendtype,
                                                       sendtype_extent, sendtype_max_extent,
                                                       own_off_counts, friend_off_counts,
                                                       cmn_nbh_mat, comm_ptr, s,
                                                       &exchange_sendbuf, &exchange_recvbuf);
            sched_bufs_from_funcs[sched_bufs_from_funcs_count++] = exchange_sendbuf;
            sched_bufs_from_funcs[sched_bufs_from_funcs_count++] = exchange_recvbuf;
            MPIR_SCHED_BARRIER(s);

            /* allocate and populate all combined messages */
            a2av_make_all_combined_msgs(t, exchange_recvbuf, friend_off_counts,
                                        sendbuf, sendcounts, sdispls, dsts,
                                        sendtype, sendtype_extent, sendtype_max_extent,
                                        cmn_nbh_mat, comm_ptr, s, &combined_sendbuf);
            sched_bufs_from_funcs[sched_bufs_from_funcs_count++] = combined_sendbuf;

            MPIR_SCHED_BARRIER(s);

            /* Schedule for the outgoing combined messages
             * (CANNOT be mixed with combined buffer building)*/
            a2av_send_to_onloaded_nbrs(t, combined_sendbuf, friend_off_counts,
                                       sendcounts, sdispls, dsts,
                                       sendtype, sendtype_extent, cmn_nbh_mat, comm_ptr, s);
            /* We can (should) free own and friend's off-counts arrays
             * because they are not part of the schedule buffers. */
            MPL_free(own_off_counts);
            MPL_free(friend_off_counts);
        }

        /* Compiling the incom_sched_mat */
        /* Scheduling necessary recv operations */
        char *rb = (char *) incom_tmp_buf;
        for (i = 0; i < indegree; i++) {
            if (!incom_sched_mat[i][0]) {
                /* On incoming neighbors */
                if (incom_sched_mat[i][1] == t) {
                    /* Schedule a receive from the corresponding source. */
                    int incom_recv_count = a2av_find_incom_recv_count(incom_sched_mat[i], srcs,
                                                                      recvcounts, indegree);
                    mpi_errno = MPIR_Sched_recv(rb, incom_recv_count, recvtype,
                                                incom_sched_mat[i][COMB_LIST_START_IDX],
                                                comm_ptr, s);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);

#ifdef SCHED_DEBUG
                    MPIR_Request *req_ptr = NULL;
                    int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
                        MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
                    mpi_errno = MPID_Irecv(rb, incom_recv_count, recvtype,
                                           incom_sched_mat[i][COMB_LIST_START_IDX],
                                           1000, comm_ptr, context_offset, &req_ptr);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                    debug_glob_all_reqs[debug_glob_all_reqs_idx++] = req_ptr->handle;
#endif

                    rb += incom_recv_count * recvtype_extent;
                }
            } else {    /* Off incoming neighbors */

                /* Nothing to do with no cumulative. */
            }
        }

        MPIR_SCHED_BARRIER(s);

#ifdef SCHED_DEBUG
        mpi_errno = MPIR_Waitall(debug_glob_all_reqs_idx, debug_glob_all_reqs, MPI_STATUS_IGNORE);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        debug_glob_all_reqs_idx = 0;    /* set index back to zero for future use */
        print_vec(comm_ptr->rank, indegree, (int *) incom_tmp_buf, "incom_tmp_buf: ");
#endif

        /* Scheduling message copies from incom_tmp_buf to final recv_buf */
        char *copy_from = (char *) incom_tmp_buf;
        char *copy_to;
        for (i = 0; i < indegree; i++) {
            if (!incom_sched_mat[i][0]) {
                /* On incoming neighbors */
                if (incom_sched_mat[i][1] == t) {
                    for (j = 0; j < incom_sched_mat[i][2]; j++) {
                        int nbr_idx = find_in_arr(srcs, indegree,
                                                  incom_sched_mat[i][j + COMB_LIST_START_IDX]);

#ifdef SCHED_DEBUG
                        sprintf(content, "t = %d, incom_nbr = %d, nbr_idx = %d, "
                                "recvcounts[nbr_idx] = %d, rdispls[nbr_idx] = %d\n",
                                t, incom_sched_mat[i][j + COMB_LIST_START_IDX], nbr_idx,
                                recvcounts[nbr_idx], rdispls[nbr_idx]);
                        print_in_file(comm_ptr->rank, content);
#endif

                        copy_to = ((char *) recvbuf) + rdispls[nbr_idx] * recvtype_extent;
                        MPIR_Sched_copy(copy_from, recvcounts[nbr_idx], recvtype,
                                        copy_to, recvcounts[nbr_idx], recvtype, s);

#ifdef SCHED_DEBUG
                        MPIR_Localcopy(copy_from, recvcounts[nbr_idx], recvtype,
                                       copy_to, recvcounts[nbr_idx], recvtype);
#endif

                        copy_from += recvcounts[nbr_idx] * recvtype_extent;
                    }
                }
            }
        }

        MPIR_SCHED_BARRIER(s);
    }

#ifdef SCHED_DEBUG
    print_vec(comm_ptr->rank, indegree, (int *) recvbuf, "recvbuf before residual send/recvs: ");
#endif

    /* Schedule a send for any residual outgoing neighbors */
    MPIR_SCHED_BARRIER(s);
    for (i = 0; i < cmn_nbh_mat->num_rows; i++) {
        if (!cmn_nbh_mat->ignore_row[i]) {
            /* out neighbor still active */
            mpi_errno = MPIR_Sched_send(((char *) sendbuf) + sdispls[i] * sendtype_extent,
                                        sendcounts[i], sendtype, dsts[i], comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

#ifdef SCHED_DEBUG
            int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
                MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
            MPIR_Request *req_ptr = NULL;
            mpi_errno = MPID_Isend(((char *) sendbuf) + sdispls[i] * sendtype_extent,
                                   sendcounts[i], sendtype, dsts[i],
                                   1000, comm_ptr, context_offset, &req_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            debug_glob_all_reqs[debug_glob_all_reqs_idx++] = req_ptr->handle;
#endif

        }
    }

    /* Schedule a recv for any residual incoming neighbors */
    /* NOTE: the incoming message can still be a combined one.
     * TODO: We might be able to use 't' to schedule these
     * residual recvs in a stepwise fashion to create some
     * overlap among the recv and copy operations.
     */
    char *rb = (char *) incom_tmp_buf;
    for (i = 0; i < indegree; i++) {
        /* ON incoming neighbors not covered before */
        if (!incom_sched_mat[i][0] && incom_sched_mat[i][1] >= cmn_nbh_mat->t) {
            /* Schedule a receive from the corresponding source. */
            int incom_recv_count = a2av_find_incom_recv_count(incom_sched_mat[i], srcs,
                                                              recvcounts, indegree);
            mpi_errno = MPIR_Sched_recv(rb, incom_recv_count, recvtype,
                                        incom_sched_mat[i][COMB_LIST_START_IDX], comm_ptr, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

#ifdef SCHED_DEBUG
            MPIR_Request *req_ptr = NULL;
            int context_offset = (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) ?
                MPIR_CONTEXT_INTRA_COLL : MPIR_CONTEXT_INTER_COLL;
            mpi_errno = MPID_Irecv(rb, incom_recv_count, recvtype,
                                   incom_sched_mat[i][COMB_LIST_START_IDX],
                                   1000, comm_ptr, context_offset, &req_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            debug_glob_all_reqs[debug_glob_all_reqs_idx++] = req_ptr->handle;
#endif

            rb += incom_recv_count * recvtype_extent;
        } else {        /* OFF incoming neighbors */

            /* Nothing to do with no cumulative. */
        }
    }

    MPIR_SCHED_BARRIER(s);

#ifdef SCHED_DEBUG
    mpi_errno = MPIR_Waitall(debug_glob_all_reqs_idx, debug_glob_all_reqs, MPI_STATUS_IGNORE);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    debug_glob_all_reqs_idx = 0;        /* set index back to zero for future use */
    print_vec(comm_ptr->rank, indegree, (int *) incom_tmp_buf,
              "incom_tmp_buf after residual recvs: ");
#endif

    /* Scheduling message copies from incom_tmp_buf to final recv_buf */
    char *copy_from = (char *) incom_tmp_buf;
    char *copy_to;
    for (i = 0; i < indegree; i++) {
        /* ON incoming neighbors not covered before */
        if (!incom_sched_mat[i][0] && incom_sched_mat[i][1] >= cmn_nbh_mat->t) {
            for (j = 0; j < incom_sched_mat[i][2]; j++) {
                int nbr_idx = find_in_arr(srcs, indegree,
                                          incom_sched_mat[i][j + COMB_LIST_START_IDX]);
                copy_to = ((char *) recvbuf) + rdispls[nbr_idx] * recvtype_extent;
                MPIR_Sched_copy(copy_from, recvcounts[nbr_idx], recvtype,
                                copy_to, recvcounts[nbr_idx], recvtype, s);

#ifdef SCHED_DEBUG
                MPIR_Localcopy(copy_from, recvcounts[nbr_idx], recvtype,
                               copy_to, recvcounts[nbr_idx], recvtype);
#endif

                copy_from += recvcounts[nbr_idx] * recvtype_extent;
            }
        }
    }

    MPIR_SCHED_BARRIER(s);

#ifdef SCHED_DEBUG
    print_vec(comm_ptr->rank, indegree, (int *) recvbuf, "Final recvbuf: ");
#endif

    MPIR_SCHED_CHKPMEM_COMMIT(s);
    while (sched_bufs_from_funcs_count > 0) {
        /* Schedule to free all bufs allocated in functions */
        mpi_errno = MPIR_Sched_cb(&MPIR_Sched_cb_free_buf,
                                  (sched_bufs_from_funcs[--sched_bufs_from_funcs_count]), s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    sched_time += MPI_Wtime();

#ifdef DEBUG
    sprintf(content, "Done with building the schedule.\n");
    print_in_file(comm_ptr->rank, content);
#endif

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
