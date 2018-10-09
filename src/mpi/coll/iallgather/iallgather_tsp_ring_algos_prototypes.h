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
