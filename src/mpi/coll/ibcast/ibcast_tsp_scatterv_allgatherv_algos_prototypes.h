/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IBCAST_TSP_SCATTERV_ALLGATHERV_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ibcast_intra_scatterv_allgatherv
#define MPIR_TSP_Ibcast_intra_scatterv_allgatherv             MPIR_TSP_NAMESPACE(Ibcast_intra_scatterv_allgatherv)
#undef MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv
#define MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv       MPIR_TSP_NAMESPACE(Ibcast_sched_intra_scatterv_allgatherv)

int MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(void *buffer, int count,
                                                    MPI_Datatype datatype, int root,
                                                    MPIR_Comm * comm, int scatterv_k,
                                                    int allgatherv_k, MPIR_TSP_sched_t * sched);
int MPIR_TSP_Ibcast_intra_scatterv_allgatherv(void *buffer, int count, MPI_Datatype datatype,
                                              int root, MPIR_Comm * comm, int scatterv_k,
                                              int allgatherv_k, MPIR_Request ** req);
