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
                                             int tag, MPIR_Comm * comm, MPIR_TSP_sched_t * sched);

int MPIR_TSP_Ialltoall_intra_scattered(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm, MPIR_Request ** req);
