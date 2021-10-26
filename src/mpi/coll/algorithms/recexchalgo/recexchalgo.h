/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef RECEXCHALGO_H_INCLUDED
#define RECEXCHALGO_H_INCLUDED

typedef struct {
    int k;
    int p_of_k;
    int T;
    bool in_step2;
    int step1_sendto;
    int step1_nrecvs;
    int *step1_recvfrom;
    int step2_nphases;
    int **step2_nbrs;

    int per_nbr_buffer;         /* if false, use a single temporary buffer */
    void **nbr_bufs;            /* temp buffer allocated with MPIR_TSP_sched_malloc */

    int is_commutative;         /* if false, step2_nbrs are sorted and myidx is set */
    int *myidxes;               /* for each phase, set to number of nbrs to the left */
} MPII_Recexchalgo_t;

int MPII_Recexchalgo_init(void);
int MPII_Recexchalgo_comm_init(MPIR_Comm * comm);
int MPII_Recexchalgo_comm_cleanup(MPIR_Comm * comm);
int MPII_Recexchalgo_start(int rank, int nranks, int k, MPII_Recexchalgo_t * recexch);
void MPII_Recexchalgo_finish(MPII_Recexchalgo_t * recexch);
/* Converts original rank to rank in step 2 of recursive exchange */
int MPII_Recexchalgo_origrank_to_step2rank(int rank, int rem, int T, int k);
/* Converts rank in step 2 of recursive exchange to original rank */
int MPII_Recexchalgo_step2rank_to_origrank(int rank, int rem, int T, int k);
int MPII_Recexchalgo_get_count_and_offset(int rank, int phase, int k, int nranks, int *count,
                                          int *offset);
int MPII_Recexchalgo_reverse_digits_step2(int rank, int comm_size, int k);

int MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(int rank, int nranks, int *nrecvs_,
                                                   int **recv_id_, int tag, void *recvbuf,
                                                   size_t recv_extent, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * displs, MPI_Datatype recvtype,
                                                   int is_dist_halving, MPIR_Comm * comm,
                                                   MPII_Recexchalgo_t * recexch,
                                                   MPIR_TSP_sched_t sched);
int MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2(void *tmp_results, void *tmp_recvbuf,
                                                       const MPI_Aint * recvcounts,
                                                       MPI_Aint * displs, MPI_Datatype datatype,
                                                       MPI_Op op, size_t extent, int tag,
                                                       MPIR_Comm * comm, int is_dist_halving,
                                                       int rank, int nranks, int sink_id,
                                                       int is_out_vtcs, int *reduce_id_,
                                                       MPII_Recexchalgo_t * recexch,
                                                       MPIR_TSP_sched_t sched);

#endif /* RECEXCHALGO_H_INCLUDED */
