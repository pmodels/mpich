/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
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
                                            MPIR_Comm * comm, MPIR_TSP_sched_t * s);

int MPIR_TSP_Ialltoallw_intra_blocked(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[], const int rdispls[],
                                      const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                      MPIR_Request ** req);
