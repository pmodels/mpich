/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLREDUCE_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ireduce_scatter_block_intra_recexch
#define MPIR_TSP_Ireduce_scatter_block_intra_recexch                      MPIR_TSP_NAMESPACE(Ireduce_scatter_block_intra_recexch)
#undef MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch
#define MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch                MPIR_TSP_NAMESPACE(Ireduce_scatter_block_sched_intra_recexch)

int MPIR_TSP_Ireduce_scatter_block_sched_intra_recexch(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm, int k,
                                                       MPIR_TSP_sched_t * sched);

int MPIR_TSP_Ireduce_scatter_block_intra_recexch(const void *sendbuf, void *recvbuf, int recvcount,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPIR_Request ** req, int k);
