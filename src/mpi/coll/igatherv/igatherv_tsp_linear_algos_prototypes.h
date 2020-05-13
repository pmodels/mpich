/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IGATHERV_TSP_LINEAR_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Igatherv_allcomm_linear
#define MPIR_TSP_Igatherv_allcomm_linear             MPIR_TSP_NAMESPACE(Igatherv_allcomm_linear)
#undef MPIR_TSP_Igatherv_sched_allcomm_linear
#define MPIR_TSP_Igatherv_sched_allcomm_linear       MPIR_TSP_NAMESPACE(Igatherv_sched_allcomm_linear)

int MPIR_TSP_Igatherv_sched_allcomm_linear(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int recvcounts[], const int displs[],
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                           MPIR_TSP_sched_t * sched);
int MPIR_TSP_Igatherv_allcomm_linear(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, const int recvcounts[], const int displs[],
                                     MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                     MPIR_Request ** request);
