/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALLV_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ialltoallv_intra_blocked
#define MPIR_TSP_Ialltoallv_intra_blocked                      MPIR_TSP_NAMESPACE(Ialltoallv_intra_blocked)
#undef MPIR_TSP_Ialltoallv_sched_intra_blocked
#define MPIR_TSP_Ialltoallv_sched_intra_blocked                MPIR_TSP_NAMESPACE(Ialltoallv_sched_intra_blocked)

int MPIR_TSP_Ialltoallv_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, int bblock, MPIR_TSP_sched_t * s);

int MPIR_TSP_Ialltoallv_intra_blocked(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                      const int recvcounts[], const int rdispls[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm, int bblock,
                                      MPIR_Request ** req);
