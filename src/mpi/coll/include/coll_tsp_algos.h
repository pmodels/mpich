/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
*  (C) 2019 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.

*  Portions of this code were written by Intel Corporation.
*  Copyright (C) 2011-2019 Intel Corporation.  Intel provides this material
*  to Argonne National Laboratory subject to Software Grant and Corporate
*  Contributor License Agreement dated February 8, 2012.
*/

/* Include guard is intentionally omitted.
* include specific tsp namespace before include this file
* e.g. #include "tsp_gentran.h"
*/

/* PARAMS_BCAST, etc. are defined in src/include/mpir_coll.h */

/* Transport based i-collective algorithm functions. */

#include "tsp_namespace_def.h"

#undef  MPIR_TSP_Ibcast_intra_tree
#define MPIR_TSP_Ibcast_intra_tree MPIR_TSP_NAMESPACE(Ibcast_intra_tree)
int MPIR_TSP_Ibcast_intra_tree(PARAMS_BCAST, int tree_type, int k, int maxbytes,
                               MPIR_Request ** req);

#undef  MPIR_TSP_Ibcast_sched_intra_tree
#define MPIR_TSP_Ibcast_sched_intra_tree MPIR_TSP_NAMESPACE(Ibcast_sched_intra_tree)
int MPIR_TSP_Ibcast_sched_intra_tree(PARAMS_BCAST, int tree_type, int k, int maxbytes,
                                     MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Ibcast_intra_scatter_recexch_allgather
#define MPIR_TSP_Ibcast_intra_scatter_recexch_allgather MPIR_TSP_NAMESPACE(Ibcast_intra_scatter_recexch_allgather)
int MPIR_TSP_Ibcast_intra_scatter_recexch_allgather(PARAMS_BCAST, MPIR_Request ** req);

#undef  MPIR_TSP_Ibcast_sched_intra_scatter_recexch_allgather
#define MPIR_TSP_Ibcast_sched_intra_scatter_recexch_allgather MPIR_TSP_NAMESPACE(Ibcast_sched_intra_scatter_recexch_allgather)
int MPIR_TSP_Ibcast_sched_intra_scatter_recexch_allgather(PARAMS_BCAST, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Igather_intra_tree
#define MPIR_TSP_Igather_intra_tree MPIR_TSP_NAMESPACE(Igather_intra_tree)
int MPIR_TSP_Igather_intra_tree(PARAMS_GATHER, int k, MPIR_Request ** req);

#undef  MPIR_TSP_Igather_sched_intra_tree
#define MPIR_TSP_Igather_sched_intra_tree MPIR_TSP_NAMESPACE(Igather_sched_intra_tree)
int MPIR_TSP_Igather_sched_intra_tree(PARAMS_GATHER, int k, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iscatter_intra_tree
#define MPIR_TSP_Iscatter_intra_tree MPIR_TSP_NAMESPACE(Iscatter_intra_tree)
int MPIR_TSP_Iscatter_intra_tree(PARAMS_SCATTER, int k, MPIR_Request ** req);

#undef  MPIR_TSP_Iscatter_sched_intra_tree
#define MPIR_TSP_Iscatter_sched_intra_tree MPIR_TSP_NAMESPACE(Iscatter_sched_intra_tree)
int MPIR_TSP_Iscatter_sched_intra_tree(PARAMS_SCATTER, int k, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgather_intra_brucks
#define MPIR_TSP_Iallgather_intra_brucks MPIR_TSP_NAMESPACE(Iallgather_intra_brucks)
int MPIR_TSP_Iallgather_intra_brucks(PARAMS_ALLGATHER, int k, MPIR_Request ** req);

#undef  MPIR_TSP_Iallgather_sched_intra_brucks
#define MPIR_TSP_Iallgather_sched_intra_brucks MPIR_TSP_NAMESPACE(Iallgather_sched_intra_brucks)
int MPIR_TSP_Iallgather_sched_intra_brucks(PARAMS_ALLGATHER, int k, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgather_intra_recexch
#define MPIR_TSP_Iallgather_intra_recexch MPIR_TSP_NAMESPACE(Iallgather_intra_recexch)
int MPIR_TSP_Iallgather_intra_recexch(PARAMS_ALLGATHER, int is_dist_halving, int k,
                                      MPIR_Request ** req);

#undef  MPIR_TSP_Iallgather_sched_intra_recexch
#define MPIR_TSP_Iallgather_sched_intra_recexch MPIR_TSP_NAMESPACE(Iallgather_sched_intra_recexch)
int MPIR_TSP_Iallgather_sched_intra_recexch(PARAMS_ALLGATHER, int is_dist_halving, int k,
                                            MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgather_sched_intra_recexch_data_exchange
#define MPIR_TSP_Iallgather_sched_intra_recexch_data_exchange MPIR_TSP_NAMESPACE(Iallgather_sched_intra_recexch_data_exchange)
int MPIR_TSP_Iallgather_sched_intra_recexch_data_exchange(int rank, int nranks, int k, int p_of_k,
                                                          int log_pofk, int T, void *recvbuf,
                                                          MPI_Datatype recvtype, size_t recv_extent,
                                                          int recvcount, int tag, MPIR_Comm * comm,
                                                          MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgather_sched_intra_recexch_step1
#define MPIR_TSP_Iallgather_sched_intra_recexch_step1 MPIR_TSP_NAMESPACE(Iallgather_sched_intra_recexch_step1)
int MPIR_TSP_Iallgather_sched_intra_recexch_step1(int step1_sendto, int *step1_recvfrom,
                                                  int step1_nrecvs, int is_inplace, int rank,
                                                  int tag, const void *sendbuf, void *recvbuf,
                                                  size_t recv_extent, int recvcount,
                                                  MPI_Datatype recvtype, int n_invtcs, int *invtx,
                                                  MPIR_Comm * comm, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgather_sched_intra_recexch_step2
#define MPIR_TSP_Iallgather_sched_intra_recexch_step2 MPIR_TSP_NAMESPACE(Iallgather_sched_intra_recexch_step2)
int MPIR_TSP_Iallgather_sched_intra_recexch_step2(int step1_sendto, int step2_nphases,
                                                  int **step2_nbrs, int rank, int nranks, int k,
                                                  int p_of_k, int log_pofk, int T, int *nrecvs_,
                                                  int **recv_id_, int tag, void *recvbuf,
                                                  size_t recv_extent, int recvcount,
                                                  MPI_Datatype recvtype, int is_dist_halving,
                                                  MPIR_Comm * comm, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgather_sched_intra_recexch_step3
#define MPIR_TSP_Iallgather_sched_intra_recexch_step3 MPIR_TSP_NAMESPACE(Iallgather_sched_intra_recexch_step3)
int MPIR_TSP_Iallgather_sched_intra_recexch_step3(int step1_sendto, int *step1_recvfrom,
                                                  int step1_nrecvs, int step2_nphases,
                                                  void *recvbuf, int recvcount, int nranks, int k,
                                                  int nrecvs, int *recv_id, int tag,
                                                  MPI_Datatype recvtype, MPIR_Comm * comm,
                                                  MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgatherv_intra_ring
#define MPIR_TSP_Iallgatherv_intra_ring MPIR_TSP_NAMESPACE(Iallgatherv_intra_ring)
int MPIR_TSP_Iallgatherv_intra_ring(PARAMS_ALLGATHERV, MPIR_Request ** req);

#undef  MPIR_TSP_Iallgatherv_sched_intra_ring
#define MPIR_TSP_Iallgatherv_sched_intra_ring MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_ring)
int MPIR_TSP_Iallgatherv_sched_intra_ring(PARAMS_ALLGATHERV, int tag, MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgatherv_intra_recexch
#define MPIR_TSP_Iallgatherv_intra_recexch MPIR_TSP_NAMESPACE(Iallgatherv_intra_recexch)
int MPIR_TSP_Iallgatherv_intra_recexch(PARAMS_ALLGATHERV, int is_dist_halving, int k,
                                       MPIR_Request ** req);

#undef  MPIR_TSP_Iallgatherv_sched_intra_recexch
#define MPIR_TSP_Iallgatherv_sched_intra_recexch MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_recexch)
int MPIR_TSP_Iallgatherv_sched_intra_recexch(PARAMS_ALLGATHERV, int is_dist_halving, int k, int tag,
                                             MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgatherv_sched_intra_recexch_data_exchange
#define MPIR_TSP_Iallgatherv_sched_intra_recexch_data_exchange MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_recexch_data_exchange)
int MPIR_TSP_Iallgatherv_sched_intra_recexch_data_exchange(int rank, int nranks, int k, int p_of_k,
                                                           int log_pofk, int T, void *recvbuf,
                                                           MPI_Datatype recvtype,
                                                           size_t recv_extent,
                                                           const int *recvcounts, const int *displs,
                                                           int tag, MPIR_Comm * comm,
                                                           MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgatherv_sched_intra_recexch_step1
#define MPIR_TSP_Iallgatherv_sched_intra_recexch_step1 MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_recexch_step1)
int MPIR_TSP_Iallgatherv_sched_intra_recexch_step1(int step1_sendto, int *step1_recvfrom,
                                                   int step1_nrecvs, int is_inplace, int rank,
                                                   int tag, const void *sendbuf, void *recvbuf,
                                                   size_t recv_extent, const int *recvcounts,
                                                   const int *displs, MPI_Datatype recvtype,
                                                   int n_invtcs, int *invtx, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgatherv_sched_intra_recexch_step2
#define MPIR_TSP_Iallgatherv_sched_intra_recexch_step2 MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_recexch_step2)
int MPIR_TSP_Iallgatherv_sched_intra_recexch_step2(int step1_sendto, int step2_nphases,
                                                   int **step2_nbrs, int rank, int nranks, int k,
                                                   int p_of_k, int log_pofk, int T, int *nrecvs_,
                                                   int **recv_id_, int tag, void *recvbuf,
                                                   size_t recv_extent, const int *recvcounts,
                                                   const int *displs, MPI_Datatype recvtype,
                                                   int is_dist_halving, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallgatherv_sched_intra_recexch_step3
#define MPIR_TSP_Iallgatherv_sched_intra_recexch_step3 MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_recexch_step3)
int MPIR_TSP_Iallgatherv_sched_intra_recexch_step3(int step1_sendto, int *step1_recvfrom,
                                                   int step1_nrecvs, int step2_nphases,
                                                   void *recvbuf, const int *recvcounts, int nranks,
                                                   int k, int nrecvs, int *recv_id, int tag,
                                                   MPI_Datatype recvtype, MPIR_Comm * comm,
                                                   MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Ireduce_intra_tree
#define MPIR_TSP_Ireduce_intra_tree MPIR_TSP_NAMESPACE(Ireduce_intra_tree)
int MPIR_TSP_Ireduce_intra_tree(PARAMS_REDUCE, int tree_type, int k, int segsize,
                                MPIR_Request ** req);

#undef  MPIR_TSP_Ireduce_sched_intra_tree
#define MPIR_TSP_Ireduce_sched_intra_tree MPIR_TSP_NAMESPACE(Ireduce_sched_intra_tree)
int MPIR_TSP_Ireduce_sched_intra_tree(PARAMS_REDUCE, int tree_type, int k, int segsize,
                                      MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallreduce_intra_tree
#define MPIR_TSP_Iallreduce_intra_tree MPIR_TSP_NAMESPACE(Iallreduce_intra_tree)
int MPIR_TSP_Iallreduce_intra_tree(PARAMS_ALLREDUCE, int tree_type, int k, int segsize,
                                   MPIR_Request ** req);

#undef  MPIR_TSP_Iallreduce_sched_intra_tree
#define MPIR_TSP_Iallreduce_sched_intra_tree MPIR_TSP_NAMESPACE(Iallreduce_sched_intra_tree)
int MPIR_TSP_Iallreduce_sched_intra_tree(PARAMS_ALLREDUCE, int tree_type, int k, int segsize,
                                         MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Iallreduce_intra_recexch
#define MPIR_TSP_Iallreduce_intra_recexch MPIR_TSP_NAMESPACE(Iallreduce_intra_recexch)
int MPIR_TSP_Iallreduce_intra_recexch(PARAMS_ALLREDUCE, int per_nbr_buffer, int k,
                                      MPIR_Request ** req);

#undef  MPIR_TSP_Iallreduce_sched_intra_recexch
#define MPIR_TSP_Iallreduce_sched_intra_recexch MPIR_TSP_NAMESPACE(Iallreduce_sched_intra_recexch)
int MPIR_TSP_Iallreduce_sched_intra_recexch(PARAMS_ALLREDUCE, int per_nbr_buffer, int k, int tag,
                                            MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Ireduce_scatter_intra_recexch
#define MPIR_TSP_Ireduce_scatter_intra_recexch MPIR_TSP_NAMESPACE(Ireduce_scatter_intra_recexch)
int MPIR_TSP_Ireduce_scatter_intra_recexch(PARAMS_REDUCE_SCATTER, int k, MPIR_Request ** req);

#undef  MPIR_TSP_Ireduce_scatter_sched_intra_recexch
#define MPIR_TSP_Ireduce_scatter_sched_intra_recexch MPIR_TSP_NAMESPACE(Ireduce_scatter_sched_intra_recexch)
int MPIR_TSP_Ireduce_scatter_sched_intra_recexch(PARAMS_REDUCE_SCATTER, int k, int tag,
                                                 MPIR_TSP_sched_t * sched);

#undef  MPIR_TSP_Ireduce_scatter_block_intra_recexch
#define MPIR_TSP_Ireduce_scatter_block_intra_recexch MPIR_TSP_NAMESPACE(Ireduce_scatter_block_intra_recexch)
int MPIR_TSP_Ireduce_scatter_block_intra_recexch(PARAMS_REDUCE_SCATTER_BLOCK, int k,
                                                 MPIR_Request ** req);

#undef  MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch
#define MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch MPIR_TSP_NAMESPACE(Ireduce_scatter_block_sched_intra_recexch)
int MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch(PARAMS_REDUCE_SCATTER_BLOCK, int k, int tag,
                                                       MPIR_TSP_sched_t * sched);
