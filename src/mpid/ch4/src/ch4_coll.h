/* -*- Mode: C ; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_BARRIER);

    ret = MPIDI_NM_mpi_barrier(comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_BARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_BCAST);

    ret = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_BCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLREDUCE);

    ret = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLGATHER);

    ret = MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLGATHERV);

    ret = MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SCATTER);

    ret = MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SCATTERV);

    ret = MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype,
                                recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GATHER);

    ret = MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_GATHERV);

    ret = MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_GATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLTOALL);

    ret = MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLTOALLV);

    ret = MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                 recvbuf, recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallw(const void *sendbuf, const int sendcounts[],
                                             const int sdispls[], const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int recvcounts[],
                                             const int rdispls[], const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ALLTOALLW);

    ret = MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                 recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce(const void *sendbuf, void *recvbuf,
                                          int count, MPI_Datatype datatype, MPI_Op op,
                                          int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REDUCE);

    ret = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REDUCE_SCATTER);

    ret = MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                      errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_REDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                            datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_REDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_SCAN);

    ret = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_SCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Exscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_EXSCAN);

    ret = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_EXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes, void *recvbuf,
                                                      const int *recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Neighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        const int *recvcounts, const int *displs,
                                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                                        MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                       const int *sdispls, MPI_Datatype sendtype,
                                                       void *recvbuf, const int *recvcounts,
                                                       const int *rdispls, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ineighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                       const MPI_Aint * sdispls,
                                                       const MPI_Datatype * sendtypes,
                                                       void *recvbuf, const int *recvcounts,
                                                       const MPI_Aint * rdispls,
                                                       const MPI_Datatype * recvtypes,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_INEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IBARRIER);

    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                          int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IBCAST);

    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IBCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iallgather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLGATHER);

    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iallgatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLGATHERV);

    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLREDUCE);

    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ialltoall(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLTOALL);

    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ialltoallv(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, MPI_Datatype sendtype,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, MPI_Datatype recvtype,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLTOALLV);

    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ialltoallw(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, const MPI_Datatype * sendtypes,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, const MPI_Datatype * recvtypes,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IALLTOALLW);

    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iexscan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IEXSCAN);

    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IEXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Igather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IGATHER);

    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Igatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IGATHERV);

    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                         int recvcount, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm,
                                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IREDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IREDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                   const int *recvcounts, MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IREDUCE_SCATTER);

    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IREDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Ireduce(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, int root,
                                           MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_IREDUCE);

    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_IREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISCAN);

    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iscatter(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISCATTER);

    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Iscatterv(const void *sendbuf, const int *sendcounts,
                                             const int *displs, MPI_Datatype sendtype,
                                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                             int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_ISCATTERV);

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_ISCATTERV);
    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */
