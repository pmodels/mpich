/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IALLREDUCE_TSP_TREE_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iallreduce_intra_tree
#define MPIR_TSP_Iallreduce_intra_tree                      MPIR_TSP_NAMESPACE(Iallreduce_intra_tree)
#undef MPIR_TSP_Iallreduce_sched_intra_tree
#define MPIR_TSP_Iallreduce_sched_intra_tree                MPIR_TSP_NAMESPACE(Iallreduce_sched_intra_tree)

int MPIR_TSP_Iallreduce_sched_intra_tree(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op,
                                         MPIR_Comm * comm, int tree_type, int k, int segsize,
                                         int buffer_per_child, MPIR_TSP_sched_t * sched);
int MPIR_TSP_Iallreduce_intra_tree(const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                   MPIR_Request ** req, int tree_type, int k, int segsize,
                                   int buffer_per_child);
