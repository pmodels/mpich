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

/* Header protection (i.e., IALLGATHERV_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iallgatherv_intra_recexch_data_exchange
#define MPIR_TSP_Iallgatherv_intra_recexch_data_exchange        MPIR_TSP_NAMESPACE(Iallgatherv_intra_recexch_data_exchange)
#undef MPIR_TSP_Iallgatherv_intra_recexch_step1
#define MPIR_TSP_Iallgatherv_intra_recexch_step1                MPIR_TSP_NAMESPACE(Iallgatherv_intra_recexch_step1)
#undef MPIR_TSP_Iallgatherv_intra_recexch_step2
#define MPIR_TSP_Iallgatherv_intra_recexch_step2                MPIR_TSP_NAMESPACE(Iallgatherv_intra_recexch_step2)
#undef MPIR_TSP_Iallgatherv_intra_recexch_step3
#define MPIR_TSP_Iallgatherv_intra_recexch_step3                MPIR_TSP_NAMESPACE(Iallgatherv_intra_recexch_step3)
#undef MPIR_TSP_Iallgatherv_intra_recexch
#define MPIR_TSP_Iallgatherv_intra_recexch                      MPIR_TSP_NAMESPACE(Iallgatherv_intra_recexch)
#undef MPIR_TSP_Iallgatherv_sched_intra_recexch
#define MPIR_TSP_Iallgatherv_sched_intra_recexch                MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_recexch)

int MPIR_TSP_Iallgatherv_sched_intra_recexch_data_exchange(int rank, int nranks, int k, int p_of_k,
                                                           int log_pofk, int T, void *recvbuf,
                                                           MPI_Datatype recvtype,
                                                           size_t recv_extent,
                                                           const int *recvcounts, const int *displs,
                                                           int tag, MPIR_Comm * comm,
                                                           MPIR_TSP_sched_t * sched);

int MPIR_TSP_Iallgatherv_sched_intra_recexch_step1(int step1_sendto, int *step1_recvfrom,
                                                   int step1_nrecvs, int is_inplace, int rank,
                                                   int tag, const void *sendbuf, void *recvbuf,
                                                   size_t recv_extent, const int *recvcounts,
                                                   const int *displs, MPI_Datatype recvtype,
                                                   int n_invtcs, int *invtx, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t * sched);

int MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(int step1_sendto, int step2_nphases,
                                                   int **step2_nbrs, int rank, int nranks, int k,
                                                   int p_of_k, int log_pofk, int T, int *nrecvs_,
                                                   int **recv_id_, int tag, void *recvbuf,
                                                   size_t recv_extent, const int *recvcounts,
                                                   const int *displs, MPI_Datatype recvtype,
                                                   int is_dist_halving, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t * sched);

int MPIR_TSP_Iallgatherv_sched_intra_recexch_step3(int step1_sendto, int *step1_recvfrom,
                                                   int step1_nrecvs, int step2_nphases,
                                                   void *recvbuf, const int *recvcounts, int nranks,
                                                   int k, int nrecvs, int *recv_id, int tag,
                                                   MPI_Datatype recvtype, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t * sched);

int MPIR_TSP_Iallgatherv_sched_intra_recexch(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, int tag, MPIR_Comm * comm,
                                             int is_dist_halving, int k, MPIR_TSP_sched_t * sched);

int MPIR_TSP_Iallgatherv_intra_recexch(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, MPIR_Comm * comm, MPIR_Request ** req,
                                       int allgatherv_type, int k);
