/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., ISCATTERV_TSP_LINEAR_ALGOS_PROTOTYPES_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "tsp_namespace_def.h"

#undef MPIR_TSP_Iscatterv_allcomm_linear
#define MPIR_TSP_Iscatterv_allcomm_linear             MPIR_TSP_NAMESPACE(Iscatterv_allcomm_linear)
#undef MPIR_TSP_Iscatterv_sched_allcomm_linear
#define MPIR_TSP_Iscatterv_sched_allcomm_linear       MPIR_TSP_NAMESPACE(Iscatterv_sched_allcomm_linear)

int MPIR_TSP_Iscatterv_sched_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                            const int displs[], MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_TSP_sched_t * sched);
int MPIR_TSP_Iscatterv_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                      const int displs[], MPI_Datatype sendtype, void *recvbuf,
                                      int recvcount, MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Request ** req);
