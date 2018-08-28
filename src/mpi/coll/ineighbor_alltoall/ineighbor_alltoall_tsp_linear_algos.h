/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., INEIGHBOR_ALLTOALL_TSP_LINEAR_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "treealgo.h"
#include "tsp_namespace_def.h"

/* Routine to schedule linear algorithm fir neighbor_alltoall */
int MPIR_TSP_Ineighbor_alltoall_sched_allcomm_linear(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm_ptr, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int k, l;
    int *srcs, *dsts;
    MPI_Aint sendtype_extent, recvtype_extent;
    int tag;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_INEIGHBOR_ALLTOALL_SCHED_ALLCOMM_LINEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_INEIGHBOR_ALLTOALL_SCHED_ALLCOMM_LINEAR);

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
        char *sb = ((char *) sendbuf) + k * sendcount * sendtype_extent;
        MPIR_TSP_sched_isend(sb, sendcount, sendtype, dsts[k], tag, comm_ptr, sched, 0, NULL);
    }

    /* receive needs to happen in the opposite order of sends.  This
     * is to cover the case of Cartesian graphs where the same process
     * might be both the left and right neighbor.  In this case, we
     * need to make sure the first message (which is from the right
     * neighbor) is received in the last buffer. */
    for (l = indegree - 1; l >= 0; l--) {
        char *rb = ((char *) recvbuf) + l * recvcount * recvtype_extent;
        MPIR_TSP_sched_irecv(rb, recvcount, recvtype, srcs[l], tag, comm_ptr, sched, 0, NULL);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_INEIGHBOR_ALLTOALL_SCHED_ALLCOMM_LINEAR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking linear algo based neighbor_allgatherv */
int MPIR_TSP_Ineighbor_alltoall_allcomm_linear(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype,
                                               MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_INEIGHBOR_ALLTOALL_ALLCOMM_LINEAR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_INEIGHBOR_ALLTOALL_ALLCOMM_LINEAR);

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched, false);

    /* schedule pipelined tree algo */
    mpi_errno = MPIR_TSP_Ineighbor_alltoall_sched_allcomm_linear(sendbuf, sendcount, sendtype,
                                                                 recvbuf, recvcount, recvtype,
                                                                 comm_ptr, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm_ptr, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_INEIGHBOR_ALLTOALL_ALLCOMM_LINEAR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
