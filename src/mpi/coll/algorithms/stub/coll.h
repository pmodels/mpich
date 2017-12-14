/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <stdio.h>
#include <string.h>

#ifndef MPIR_ALGO_NAMESPACE
#error "The collectives template must be namespaced with MPIR_ALGO_NAMESPACE"
#endif



MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_init()
{
    return 0;
}


MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_comm_init(MPIR_ALGO_comm_t * comm, int *tag, int rank, int size)
{
    MPIR_TSP_comm_init(&comm->tsp_comm, MPIR_ALGO_COMM_BASE(comm));
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_comm_init_null(MPIR_ALGO_comm_t * comm)
{
    MPIR_TSP_comm_init_null(&comm->tsp_comm, MPIR_ALGO_COMM_BASE(comm));
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_comm_cleanup(MPIR_ALGO_comm_t * comm)
{
    MPIR_TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_allgather(const void *sendbuf, int sendcount, MPIR_ALGO_dt_t sendtype,
                               void *recvbuf, int recvcount, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm,
                               int k, int halving, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_allgatherv(const void *sendbuf, int sendcount, MPIR_ALGO_dt_t sendtype,
                                void *recvbuf, const int *recvcounts, const int *displs,
                                MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_allreduce(const void *sendbuf, void *recvbuf, int count,
                               MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_alltoall(const void *sendbuf, int sendcount, MPIR_ALGO_dt_t sendtype, void *recvbuf,
                              int recvcount, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_alltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
                               MPIR_ALGO_dt_t sendtype, void *recvbuf, const int *recvcnts,
                               const int *rdispls, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm,
                               int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_alltoallw(const void *sendbuf, const int *sendcnts, const int *sdispls,
                               const MPIR_ALGO_dt_t * sendtypes, void *recvbuf, const int *recvcnts,
                               const int *rdispls, const MPIR_ALGO_dt_t * recvtypes, MPIR_ALGO_comm_t * comm,
                               int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_bcast(void *buffer, int count, MPIR_ALGO_dt_t datatype, int root,
                           MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_exscan(const void *sendbuf, void *recvbuf, int count,
                            MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_gather(const void *sendbuf, int sendcnt, MPIR_ALGO_dt_t sendtype, void *recvbuf,
                            int recvcnt, MPIR_ALGO_dt_t recvtype, int root, MPIR_ALGO_comm_t * comm,
                            int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_gatherv(const void *sendbuf, int sendcnt, MPIR_ALGO_dt_t sendtype, void *recvbuf,
                             const int *recvcnts, const int *displs, MPIR_ALGO_dt_t recvtype, int root,
                             MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                    MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, MPIR_ALGO_comm_t comm,
                                    int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount,
                                          MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm,
                                          int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}


MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_reduce(const void *sendbuf, void *recvbuf, int count, MPIR_ALGO_dt_t datatype,
                            MPIR_ALGO_op_t op, int root, MPIR_ALGO_comm_t * comm, int *errflag, int k,
                            int segsize, int nbuffers)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_scan(const void *sendbuf, void *recvbuf, int count,
                          MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_scatter(const void *sendbuf, int sendcnt, MPIR_ALGO_dt_t sendtype, void *recvbuf,
                             int recvcnt, MPIR_ALGO_dt_t recvtype, int root, MPIR_ALGO_comm_t * comm,
                             int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_scatterv(const void *sendbuf, const int *sendcnts, const int *displs,
                              MPIR_ALGO_dt_t sendtype, void *recvbuf, int recvcnt, MPIR_ALGO_dt_t recvtype,
                              int root, MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_barrier(MPIR_ALGO_comm_t * comm, int *errflag)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iallgather(const void *sendbuf, int sendcount, MPIR_ALGO_dt_t sendtype,
                                void *recvbuf, int recvcount, MPIR_ALGO_dt_t recvtype,
                                MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iallgatherv(const void *sendbuf, int sendcount, MPIR_ALGO_dt_t sendtype,
                                 void *recvbuf, const int *recvcounts, const int *displs,
                                 MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}


MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_kick_nb(MPIR_COLL_queue_elem_t * elem)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iallreduce(const void *sendbuf, void *recvbuf, int count, MPIR_ALGO_dt_t datatype,
                                MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ialltoall(const void *sendbuf, int sendcount, MPIR_ALGO_dt_t sendtype,
                               void *recvbuf, int recvcount, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm,
                               MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ialltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
                                MPIR_ALGO_dt_t sendtype, void *recvbuf, const int *recvcnts,
                                const int *rdispls, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t comm,
                                MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ialltoallw(const void *sendbuf, const int *sendcnts, const int *sdispls,
                                const MPIR_ALGO_dt_t * sendtypes, void *recvbuf, const int *recvcnts,
                                const int *rdispls, const MPIR_ALGO_dt_t * recvtypes, MPIR_ALGO_comm_t * comm,
                                MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ibcast(void *buffer, int count, MPIR_ALGO_dt_t datatype,
                            int root, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iexscan(const void *sendbuf, void *recvbuf, int count, MPIR_ALGO_dt_t datatype,
                             MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_igather(const void *sendbuf, int sendcnt, MPIR_ALGO_dt_t sendtype, void *recvbuf,
                             int recvcnt, MPIR_ALGO_dt_t recvtype, int root, MPIR_ALGO_comm_t * comm,
                             MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_igatherv(const void *sendbuf, int sendcnt, MPIR_ALGO_dt_t sendtype, void *recvbuf,
                              const int *recvcnts, const int *displs, MPIR_ALGO_dt_t recvtype,
                              int root, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ireduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcnts,
                                     MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm,
                                     MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ireduce_scatter_block(const void *sendbuf,
                                           void *recvbuf,
                                           int recvcount,
                                           MPIR_ALGO_dt_t datatype,
                                           MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ireduce(const void *sendbuf,
                             void *recvbuf,
                             int count,
                             MPIR_ALGO_dt_t datatype,
                             MPIR_ALGO_op_t op, int root, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iscan(const void *sendbuf,
                           void *recvbuf,
                           int count,
                           MPIR_ALGO_dt_t datatype,
                           MPIR_ALGO_op_t op, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iscatter(const void *sendbuf,
                              int sendcnt,
                              MPIR_ALGO_dt_t sendtype,
                              void *recvbuf,
                              int recvcnt,
                              MPIR_ALGO_dt_t recvtype,
                              int root, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_iscatterv(const void *sendbuf,
                               const int *sendcnts,
                               const int *displs,
                               MPIR_ALGO_dt_t sendtype,
                               void *recvbuf,
                               int recvcnt,
                               MPIR_ALGO_dt_t recvtype,
                               int root, MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ibarrier(MPIR_ALGO_comm_t * comm, MPIR_ALGO_req_t * request)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_neighbor_allgather(const void *sendbuf,
                                        int sendcount,
                                        MPIR_ALGO_dt_t sendtype,
                                        void *recvbuf,
                                        int recvcount, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_neighbor_allgatherv(const void *sendbuf,
                                         int sendcount,
                                         MPIR_ALGO_dt_t sendtype,
                                         void *recvbuf,
                                         const int recvcounts[],
                                         const int displs[], MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_neighbor_alltoall(const void *sendbuf,
                                       int sendcount,
                                       MPIR_ALGO_dt_t sendtype,
                                       void *recvbuf,
                                       int recvcount, MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm)
{
    MPIR_COLL_Assert(0);
    return 0;
}


MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_neighbor_alltoallv(const void *sendbuf,
                                        const int sendcounts[],
                                        const int sdispls[],
                                        MPIR_ALGO_dt_t sendtype,
                                        void *recvbuf,
                                        const int recvcounts[],
                                        const int rdispls[], MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_neighbor_alltoallw(const void *sendbuf,
                                        const int sendcounts[],
                                        const MPIR_ALGO_aint_t * sdispls[],
                                        const MPIR_ALGO_dt_t sendtypes[],
                                        void *recvbuf,
                                        const int recvcounts[],
                                        const MPIR_ALGO_aint_t * rdispls[],
                                        const MPIR_ALGO_dt_t recvtypes[], MPIR_ALGO_comm_t * comm)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ineighbor_allgather(const void *sendbuf,
                                         int sendcount,
                                         MPIR_ALGO_dt_t sendtype,
                                         void *recvbuf,
                                         int recvcount,
                                         MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, MPIR_ALGO_sched_t * s)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ineighbor_allgatherv(const void *sendbuf,
                                          int sendcount,
                                          MPIR_ALGO_dt_t sendtype,
                                          void *recvbuf,
                                          const int recvcounts[],
                                          const int displs[],
                                          MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, MPIR_ALGO_sched_t * s)
{
    MPIR_COLL_Assert(0);
    return 0;
}


MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ineighbor_alltoall(const void *sendbuf,
                                        int sendcount,
                                        MPIR_ALGO_dt_t sendtype,
                                        void *recvbuf,
                                        int recvcount,
                                        MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, MPIR_ALGO_sched_t * s)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ineighbor_alltoallv(const void *sendbuf,
                                         const int sendcounts[],
                                         const int sdispls[],
                                         MPIR_ALGO_dt_t sendtype,
                                         void *recvbuf,
                                         const int recvcounts[],
                                         const int rdispls[],
                                         MPIR_ALGO_dt_t recvtype, MPIR_ALGO_comm_t * comm, MPIR_ALGO_sched_t * s)
{
    MPIR_COLL_Assert(0);
    return 0;
}

MPL_UNUSED MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_ineighbor_alltoallw(const void *sendbuf,
                                         const int sendcounts[],
                                         const MPIR_ALGO_aint_t * sdispls[],
                                         const MPIR_ALGO_dt_t sendtypes[],
                                         void *recvbuf,
                                         const int recvcounts[],
                                         const MPIR_ALGO_aint_t * rdispls[],
                                         const MPIR_ALGO_dt_t recvtypes[],
                                         MPIR_ALGO_comm_t * comm, MPIR_ALGO_sched_t * s)
{
    MPIR_COLL_Assert(0);
    return 0;
}
