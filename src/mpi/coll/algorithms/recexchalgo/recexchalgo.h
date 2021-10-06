/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef RECEXCHALGO_H_INCLUDED
#define RECEXCHALGO_H_INCLUDED

int MPII_Recexchalgo_init(void);
int MPII_Recexchalgo_comm_init(MPIR_Comm * comm);
int MPII_Recexchalgo_comm_cleanup(MPIR_Comm * comm);
int MPII_Recexchalgo_get_neighbors(int rank, int nranks, int *k_, int *step1_sendto,
                                   int **step1_recvfrom_, int *step1_nrecvs,
                                   int ***step2_nbrs_, int *step2_nphases, int *p_of_k_, int *T_);
/* Converts original rank to rank in step 2 of recursive exchange */
int MPII_Recexchalgo_origrank_to_step2rank(int rank, int rem, int T, int k);
/* Converts rank in step 2 of recursive exchange to original rank */
int MPII_Recexchalgo_step2rank_to_origrank(int rank, int rem, int T, int k);
int MPII_Recexchalgo_get_count_and_offset(int rank, int phase, int k, int nranks, int *count,
                                          int *offset);
int MPII_Recexchalgo_reverse_digits_step2(int rank, int comm_size, int k);

int MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(int step1_sendto, int step2_nphases,
                                                   int **step2_nbrs, int rank, int nranks, int k,
                                                   int p_of_k, int log_pofk, int T, int *nrecvs_,
                                                   int **recv_id_, int tag, void *recvbuf,
                                                   size_t recv_extent, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * displs, MPI_Datatype recvtype,
                                                   int is_dist_halving, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t sched);
int MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2(void *tmp_results, void *tmp_recvbuf,
                                                       const MPI_Aint * recvcounts,
                                                       MPI_Aint * displs, MPI_Datatype datatype,
                                                       MPI_Op op, size_t extent, int tag,
                                                       MPIR_Comm * comm, int k, int is_dist_halving,
                                                       int step2_nphases, int **step2_nbrs,
                                                       int rank, int nranks, int sink_id,
                                                       int is_out_vtcs, int *reduce_id_,
                                                       MPIR_TSP_sched_t sched);

#endif /* RECEXCHALGO_H_INCLUDED */
