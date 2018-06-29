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

/* Header protection (i.e., IBCAST_TSP_TREE_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Ibcast_intra_tree
#define MPIR_TSP_Ibcast_intra_tree                      MPIR_TSP_NAMESPACE(Ibcast_intra_tree)
#undef MPIR_TSP_Ibcast_sched_intra_tree
#define MPIR_TSP_Ibcast_sched_intra_tree                MPIR_TSP_NAMESPACE(Ibcast_sched_intra_tree)

int MPIR_TSP_Ibcast_sched_intra_tree(void *buffer, int count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm, int tree_type, int k, int segsize,
                                     MPIR_TSP_sched_t * sched);
int MPIR_TSP_Ibcast_intra_tree(void *buffer, int count, MPI_Datatype datatype, int root,
                               MPIR_Comm * comm, MPIR_Request ** req, int tree_type, int k,
                               int segsize);
