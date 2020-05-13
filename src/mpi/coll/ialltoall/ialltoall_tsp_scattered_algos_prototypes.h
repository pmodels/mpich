/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLTOALL_TSP_SCATTERED_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ialltoall_intra_scattered
#define MPIR_TSP_Ialltoall_intra_scattered                 MPIR_TSP_NAMESPACE(Ialltoall_intra_scattered)
#undef MPIR_TSP_Ialltoall_sched_intra_scattered
#define MPIR_TSP_Ialltoall_sched_intra_scattered           MPIR_TSP_NAMESPACE(Ialltoall_sched_intra_scattered)

int MPIR_TSP_Ialltoall_sched_intra_scattered(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, int batch_size, int bblock,
                                             MPIR_TSP_sched_t * sched);

int MPIR_TSP_Ialltoall_intra_scattered(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm, int batch_size, int bblock,
                                       MPIR_Request ** req);
