/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALLW_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ialltoallw_intra_inplace
#define MPIR_TSP_Ialltoallw_intra_inplace                      MPIR_TSP_NAMESPACE(Ialltoallw_intra_inplace)
#undef MPIR_TSP_Ialltoallw_sched_intra_inplace
#define MPIR_TSP_Ialltoallw_sched_intra_inplace                MPIR_TSP_NAMESPACE(Ialltoallw_sched_intra_inplace)

int MPIR_TSP_Ialltoallw_sched_intra_inplace(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_TSP_sched_t * s);

int MPIR_TSP_Ialltoallw_intra_inplace(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[], const int rdispls[],
                                      const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                      MPIR_Request ** req);
