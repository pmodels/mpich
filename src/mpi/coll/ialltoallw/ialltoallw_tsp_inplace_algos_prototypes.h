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

/* Header protection (i.e., IALLTOALLV_TSP_ALGOS_PROTOTYPES_H_INCLUDED) is
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
                                            int tag, MPIR_Comm * comm, MPIR_Sched_t s);

int MPIR_TSP_Ialltoallw_intra_inplace(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[], const int rdispls[],
                                      const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                      MPIR_Request ** req);
