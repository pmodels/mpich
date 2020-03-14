/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLGATHERV_TSP_BRUCKS_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iallgatherv_intra_brucks
#define MPIR_TSP_Iallgatherv_intra_brucks                 MPIR_TSP_NAMESPACE(Iallgatherv_intra_brucks)
#undef MPIR_TSP_Iallgatherv_sched_intra_brucks
#define MPIR_TSP_Iallgatherv_sched_intra_brucks           MPIR_TSP_NAMESPACE(Iallgatherv_sched_intra_brucks)

int MPIR_TSP_Iallgatherv_sched_intra_brucks(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int recvcounts[], const int displs[],
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_TSP_sched_t * s, int k);
int MPIR_TSP_Iallgatherv_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int recvcounts[], const int displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                      MPIR_Request ** request, int k);
