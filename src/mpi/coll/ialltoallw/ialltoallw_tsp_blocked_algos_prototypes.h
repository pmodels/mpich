/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALLW_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ialltoallw_intra_blocked
#define MPIR_TSP_Ialltoallw_intra_blocked                      MPIR_TSP_NAMESPACE(Ialltoallw_intra_blocked)
#undef MPIR_TSP_Ialltoallw_sched_intra_blocked
#define MPIR_TSP_Ialltoallw_sched_intra_blocked                MPIR_TSP_NAMESPACE(Ialltoallw_sched_intra_blocked)

int MPIR_TSP_Ialltoallw_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, int bblock, MPIR_TSP_sched_t * s);

int MPIR_TSP_Ialltoallw_intra_blocked(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[], const int rdispls[],
                                      const MPI_Datatype recvtypes[], MPIR_Comm * comm, int bblock,
                                      MPIR_Request ** req);
