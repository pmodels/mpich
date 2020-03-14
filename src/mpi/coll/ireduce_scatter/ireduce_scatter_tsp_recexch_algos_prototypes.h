/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IREDUCE_SCATTER_TSP_RECEXCH_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ireduce_scatter_intra_recexch
#define MPIR_TSP_Ireduce_scatter_intra_recexch                      MPIR_TSP_NAMESPACE(Ireduce_scatter_intra_recexch)
#undef MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2
#define MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2                MPIR_TSP_NAMESPACE(Ireduce_scatter_sched_intra_recexch_step2)
#undef MPIR_TSP_Ireduce_scatter_sched_intra_recexch
#define MPIR_TSP_Ireduce_scatter_sched_intra_recexch                MPIR_TSP_NAMESPACE(Ireduce_scatter_sched_intra_recexch)

int MPIR_TSP_Ireduce_scatter_sched_intra_recexch_step2(void *tmp_results, void *tmp_recvbuf,
                                                       const int *recvcounts, int *displs,
                                                       MPI_Datatype datatype, MPI_Op op,
                                                       size_t extent, int tag, MPIR_Comm * comm,
                                                       int k, int is_dist_halving,
                                                       int step2_nphases, int **step2_nbrs,
                                                       int rank, int nranks, int sink_id,
                                                       int is_out_vtcs, int *reduce_id_,
                                                       MPIR_TSP_sched_t * sched);

int MPIR_TSP_Ireduce_scatter_sched_intra_recexch(const void *sendbuf, void *recvbuf,
                                                 const int *recvcounts, MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm, int k,
                                                 int is_dist_halving, MPIR_TSP_sched_t * sched);

int MPIR_TSP_Ireduce_scatter_intra_recexch(const void *sendbuf, void *recvbuf,
                                           const int *recvcounts, MPI_Datatype datatype, MPI_Op op,
                                           MPIR_Comm * comm, MPIR_Request ** req, int k,
                                           int rs_type);
