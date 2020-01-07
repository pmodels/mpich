/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., INEIGHBOR_ALLTOALL_TSP_LINEAR_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "treealgo.h"
#include "tsp_namespace_def.h"

/* Routine to schedule linear algorithm for neighbor_alltoallv */
int MPIR_TSP_Ineighbor_alltoallv_sched_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                                      const int sdispls[], MPI_Datatype sendtype,
                                                      void *recvbuf, const int recvcounts[],
                                                      const int rdispls[], MPI_Datatype recvtype,
                                                      MPIR_Comm * comm_ptr,
                                                      MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int k, l;
    int *srcs, *dsts;
    int tag;
    MPI_Aint sendtype_extent, recvtype_extent;




    MPIR_CHKLMEM_DECL(2);

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    MPIR_ERR_CHECK(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, int *, indegree * sizeof(int), mpi_errno, "srcs", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(dsts, int *, outdegree * sizeof(int), mpi_errno, "dsts", MPL_MEM_COMM);
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    MPIR_ERR_CHECK(mpi_errno);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb = ((char *) sendbuf) + sdispls[k] * sendtype_extent;
        MPIR_TSP_sched_isend(sb, sendcounts[k], sendtype, dsts[k], tag, comm_ptr, sched, 0, NULL);
    }

    for (l = 0; l < indegree; ++l) {
        char *rb = ((char *) recvbuf) + rdispls[l] * recvtype_extent;
        MPIR_TSP_sched_irecv(rb, recvcounts[l], recvtype, srcs[l], tag, comm_ptr, sched, 0, NULL);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking linear algo based neighbor_alltoallv */
int MPIR_TSP_Ineighbor_alltoallv_allcomm_linear(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], MPI_Datatype sendtype,
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], MPI_Datatype recvtype,
                                                MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;




    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* schedule pipelined tree algo */
    mpi_errno =
        MPIR_TSP_Ineighbor_alltoallv_sched_allcomm_linear(sendbuf, sendcounts, sdispls, sendtype,
                                                          recvbuf, recvcounts, rdispls, recvtype,
                                                          comm_ptr, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
