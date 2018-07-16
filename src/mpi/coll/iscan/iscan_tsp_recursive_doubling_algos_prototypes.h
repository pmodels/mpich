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

/* Header protection (i.e., ISCAN_TSP_RECURSIVE_DOUBLING_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iscan_intra_recursive_doubling
#define MPIR_TSP_Iscan_intra_recursive_doubling                      MPIR_TSP_NAMESPACE(Iscan_intra_recursive_doubling)
#undef MPIR_TSP_Iscan_sched_intra_recursive_doubling
#define MPIR_TSP_Iscan_sched_intra_recursive_doubling                MPIR_TSP_NAMESPACE(Iscan_sched_intra_recursive_doubling)

int MPIR_TSP_Iscan_sched_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                  MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm_ptr, MPIR_TSP_sched_t * s);

int MPIR_TSP_Iscan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op,
                                            MPIR_Comm * comm_ptr, MPIR_Request ** req);
