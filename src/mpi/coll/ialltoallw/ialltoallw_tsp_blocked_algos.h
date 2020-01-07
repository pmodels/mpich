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

/* Header protection (i.e., IALLTOALLW_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a recursive exchange based alltoallv */
int MPIR_TSP_Ialltoallw_sched_intra_blocked(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int tag;
    size_t sendtype_size, recvtype_size;
    int nranks, rank;
    int i, j, bblock, comm_block, dst;




    MPIR_Assert(sendbuf != MPI_IN_PLACE);

    nranks = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    bblock = MPIR_CVAR_ALLTOALL_THROTTLE;
    if (bblock == 0)
        bblock = nranks;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* post only bblock isends/irecvs at a time as suggested by Tony Ladd */
    for (i = 0; i < nranks; i += bblock) {
        comm_block = nranks - i < bblock ? nranks - i : bblock;

        /* do the communication -- post ss sends and receives: */
        for (j = 0; j < comm_block; j++) {
            dst = (rank + j + i) % nranks;
            if (recvcounts[dst]) {
                MPIR_Datatype_get_size_macro(recvtypes[dst], recvtype_size);
                if (recvtype_size) {
                    MPIR_TSP_sched_irecv((char *) recvbuf + rdispls[dst],
                                         recvcounts[dst], recvtypes[dst], dst, tag, comm, sched,
                                         0, NULL);
                }
            }
        }

        for (j = 0; j < comm_block; j++) {
            dst = (rank - j - i + nranks) % nranks;
            if (sendcounts[dst]) {
                MPIR_Datatype_get_size_macro(sendtypes[dst], sendtype_size);
                if (sendtype_size) {
                    MPIR_TSP_sched_isend((char *) sendbuf + sdispls[dst],
                                         sendcounts[dst], sendtypes[dst], dst, tag, comm, sched,
                                         0, NULL);
                }
            }
        }

        /* force our block of sends/recvs to complete before starting the next block */
        MPIR_TSP_sched_fence(sched);
    }

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking blocked based ALLTOALLW */
int MPIR_TSP_Ialltoallw_intra_blocked(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[], const int rdispls[],
                                      const MPI_Datatype recvtypes[], MPIR_Comm * comm,
                                      MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;
    *req = NULL;





    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!sched, mpi_errno, MPI_ERR_OTHER, "**nomem");
    MPIR_TSP_sched_create(sched);

    mpi_errno =
        MPIR_TSP_Ialltoallw_sched_intra_blocked(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                                recvcounts, rdispls, recvtypes, comm, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
