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
#include "ch4_coll_select.h"

MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BARRIER);

    ch4_algo_parameters_container = MPIDI_CH4_Barrier_select(comm, errflag);

    mpi_errno =
        MPIDI_CH4_Barrier_call(comm, errflag, ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BARRIER);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BCAST);

    ch4_algo_parameters_container = MPIDI_CH4_Bcast_select(buffer, count, datatype, root, comm, errflag);

    mpi_errno =
        MPIDI_CH4_Bcast_call(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BCAST);

    return mpi_errno;

}

MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret = MPI_SUCCESS;
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLREDUCE);

    ch4_algo_parameters_container =
        MPIDI_CH4_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    ret = MPIDI_CH4_Allreduce_call(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                   ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHER);

    ret = MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHERV);

    ret = MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTER);

    ret = MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTERV);

    ret = MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype,
                                recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHER);

    ret = MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHERV);

    ret = MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALL);

    ret = MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLV);

    ret = MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                 recvbuf, recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *sendbuf, const int sendcounts[],
                                             const int sdispls[], const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int recvcounts[],
                                             const int rdispls[], const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLW);

    ret = MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                 recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *sendbuf, void *recvbuf,
                                         int count, MPI_Datatype datatype, MPI_Op op,
                                         int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret = MPI_SUCCESS;
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE);

    ch4_algo_parameters_container =
        MPIDI_CH4_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    ret = MPIDI_CH4_Reduce_call(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag,
                                ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER);

    ret = MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                      errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                            datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCAN);

    ret = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_EXSCAN);

    ret = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_EXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes, void *recvbuf,
                                                      const int *recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        const int *recvcounts, const int *displs,
                                                        MPI_Datatype recvtype, MPIR_Comm * comm,
                                                        MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                       const int *sdispls, MPI_Datatype sendtype,
                                                       void *recvbuf, const int *recvcounts,
                                                       const int *rdispls, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                       const MPI_Aint * sdispls,
                                                       const MPI_Datatype * sendtypes,
                                                       void *recvbuf, const int *recvcounts,
                                                       const MPI_Aint * rdispls,
                                                       const MPI_Datatype * recvtypes,
                                                       MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBARRIER);

    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                          int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBCAST);

    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLGATHER);

    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLGATHERV);

    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                              MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLREDUCE);

    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALL);

    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, MPI_Datatype sendtype,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, MPI_Datatype recvtype,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALLV);

    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, const MPI_Datatype * sendtypes,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, const MPI_Datatype * recvtypes,
                                              MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALLW);

    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iexscan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IEXSCAN);

    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IEXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IGATHER);

    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IGATHERV);

    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                         int recvcount, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm,
                                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                   const int *recvcounts, MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE_SCATTER);

    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, int root,
                                           MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE);

    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCAN);

    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatter(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                            MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTER);

    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv(const void *sendbuf, const int *sendcounts,
                                             const int *displs, MPI_Datatype sendtype,
                                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                             int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTERV);

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTERV);
    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */
