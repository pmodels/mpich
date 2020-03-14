/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLGATHER_TSP_RING_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iallgather_intra_ring
#define MPIR_TSP_Iallgather_intra_ring                      MPIR_TSP_NAMESPACE(Iallgather_intra_ring)
#undef MPIR_TSP_Iallgather_sched_intra_ring
#define MPIR_TSP_Iallgather_sched_intra_ring                MPIR_TSP_NAMESPACE(Iallgather_sched_intra_ring)

int MPIR_TSP_Iallgather_sched_intra_ring(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, MPIR_Comm * comm,
                                         MPIR_TSP_sched_t * sched);

int MPIR_TSP_Iallgather_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm, MPIR_Request ** req);
