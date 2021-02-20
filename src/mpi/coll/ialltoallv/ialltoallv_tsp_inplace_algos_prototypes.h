/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALLV_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ialltoallv_intra_inplace
#define MPIR_TSP_Ialltoallv_intra_inplace                      MPIR_TSP_NAMESPACE(Ialltoallv_intra_inplace)
#undef MPIR_TSP_Ialltoallv_sched_intra_inplace
#define MPIR_TSP_Ialltoallv_sched_intra_inplace                MPIR_TSP_NAMESPACE(Ialltoallv_sched_intra_inplace)

int MPIR_TSP_Ialltoallv_sched_intra_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                            const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                            void *recvbuf, const MPI_Aint recvcounts[],
                                            const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_TSP_sched_t * s);

int MPIR_TSP_Ialltoallv_intra_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                      const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                      void *recvbuf, const MPI_Aint recvcounts[],
                                      const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                      MPIR_Comm * comm, MPIR_Request ** req);
