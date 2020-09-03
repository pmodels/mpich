/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef STUBSHM_COLL_H_INCLUDED
#define STUBSHM_COLL_H_INCLUDED

#include "stubshm_impl.h"
#include "ch4_impl.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_barrier(MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag,
                                                       const void *algo_parameters_container
                                                       ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_BARRIER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_BARRIER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                                     int root, MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag,
                                                     const void *algo_parameters_container
                                                     ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_BCAST);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_BCAST);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_allreduce(const void *sendbuf, void *recvbuf,
                                                         int count, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         const void *algo_parameters_container
                                                         ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLREDUCE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLREDUCE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_allgather(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         const void *algo_parameters_container
                                                         ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLGATHER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLGATHER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int *recvcounts, const int *displs,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag,
                                                          const void *algo_parameters_container
                                                          ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLGATHERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLGATHERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_gather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      int root, MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag,
                                                      const void *algo_parameters_container
                                                      ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_GATHER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_GATHER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_gatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, int root,
                                                       MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag,
                                                       const void *algo_parameters_container
                                                       ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_GATHERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_GATHERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_scatter(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       int root, MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag,
                                                       const void *algo_parameters_container
                                                       ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SCATTER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SCATTER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                                        const int *displs, MPI_Datatype sendtype,
                                                        void *recvbuf, int recvcount,
                                                        MPI_Datatype recvtype, int root,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag,
                                                        const void *algo_parameters_container
                                                        ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SCATTERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SCATTERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_alltoall(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag,
                                                        const void *algo_parameters_container
                                                        ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALL);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALL);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                                         const int *sdispls, MPI_Datatype sendtype,
                                                         void *recvbuf, const int *recvcounts,
                                                         const int *rdispls, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         const void *algo_parameters_container
                                                         ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALLV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALLV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_alltoallw(const void *sendbuf,
                                                         const int sendcounts[],
                                                         const int sdispls[],
                                                         const MPI_Datatype sendtypes[],
                                                         void *recvbuf, const int recvcounts[],
                                                         const int rdispls[],
                                                         const MPI_Datatype recvtypes[],
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         const void *algo_parameters_container
                                                         ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALLW);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ALLTOALLW);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op, int root,
                                                      MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag,
                                                      const void *algo_parameters_container
                                                      ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                              const int recvcounts[],
                                                              MPI_Datatype datatype, MPI_Op op,
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Errflag_t * errflag,
                                                              const void *algo_parameters_container
                                                              ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE_SCATTER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE_SCATTER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_reduce_scatter_block(const void *sendbuf,
                                                                    void *recvbuf, int recvcount,
                                                                    MPI_Datatype datatype,
                                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const void
                                                                    *algo_parameters_container
                                                                    ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE_SCATTER_BLOCK);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_REDUCE_SCATTER_BLOCK);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                                    MPI_Datatype datatype, MPI_Op op,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container
                                                    ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_SCAN);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_SCAN);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag,
                                                      const void *algo_parameters_container
                                                      ATTRIBUTE((unused)))
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_EXSCAN);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_EXSCAN);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_neighbor_allgather(const void *sendbuf,
                                                                  int sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, int recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLGATHER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLGATHER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_neighbor_allgatherv(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const int recvcounts[],
                                                                   const int displs[],
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLGATHERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLGATHERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                                 MPI_Datatype sendtype,
                                                                 void *recvbuf, int recvcount,
                                                                 MPI_Datatype recvtype,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALL);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALL);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                                  const int sendcounts[],
                                                                  const int sdispls[],
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf,
                                                                  const int recvcounts[],
                                                                  const int rdispls[],
                                                                  MPI_Datatype recvtype,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALLV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALLV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                                  const int sendcounts[],
                                                                  const MPI_Aint sdispls[],
                                                                  const MPI_Datatype sendtypes[],
                                                                  void *recvbuf,
                                                                  const int recvcounts[],
                                                                  const MPI_Aint rdispls[],
                                                                  const MPI_Datatype recvtypes[],
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALLW);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_NEIGHBOR_ALLTOALLW);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ineighbor_allgather(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf, int recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLGATHER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLGATHER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ineighbor_allgatherv(const void *sendbuf,
                                                                    int sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    const int recvcounts[],
                                                                    const int displs[],
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLGATHERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLGATHERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ineighbor_alltoall(const void *sendbuf,
                                                                  int sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, int recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALL);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALL);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                                   const int sendcounts[],
                                                                   const int sdispls[],
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const int recvcounts[],
                                                                   const int rdispls[],
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALLV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALLV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                                   const int sendcounts[],
                                                                   const MPI_Aint sdispls[],
                                                                   const MPI_Datatype sendtypes[],
                                                                   void *recvbuf,
                                                                   const int recvcounts[],
                                                                   const MPI_Aint rdispls[],
                                                                   const MPI_Datatype recvtypes[],
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALLW);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_INEIGHBOR_ALLTOALLW);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ibarrier(MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IBARRIER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IBARRIER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ibcast(void *buffer, int count,
                                                      MPI_Datatype datatype, int root,
                                                      MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IBCAST);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IBCAST);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iallgather(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          int recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IALLGATHER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IALLGATHER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           const int *recvcounts, const int *displs,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IALLGATHERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IALLGATHERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALL);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALL);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ialltoallv(const void *sendbuf,
                                                          const int *sendcounts, const int *sdispls,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int *recvcounts, const int *rdispls,
                                                          MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALLV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALLV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ialltoallw(const void *sendbuf,
                                                          const int *sendcounts, const int *sdispls,
                                                          const MPI_Datatype sendtypes[],
                                                          void *recvbuf, const int *recvcounts,
                                                          const int *rdispls,
                                                          const MPI_Datatype recvtypes[],
                                                          MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALLW);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IALLTOALLW);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iexscan(const void *sendbuf, void *recvbuf,
                                                       int count, MPI_Datatype datatype, MPI_Op op,
                                                       MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IEXSCAN);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IEXSCAN);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_igather(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       int root, MPIR_Comm * comm_ptr,
                                                       MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IGATHER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IGATHER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_igatherv(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        const int *recvcounts, const int *displs,
                                                        MPI_Datatype recvtype, int root,
                                                        MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IGATHERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IGATHERV);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ireduce_scatter_block(const void *sendbuf,
                                                                     void *recvbuf, int recvcount,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE_SCATTER_BLOCK);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE_SCATTER_BLOCK);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                               const int recvcounts[],
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE_SCATTER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE_SCATTER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_ireduce(const void *sendbuf, void *recvbuf,
                                                       int count, MPI_Datatype datatype, MPI_Op op,
                                                       int root, MPIR_Comm * comm_ptr,
                                                       MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IREDUCE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iallreduce(const void *sendbuf, void *recvbuf,
                                                          int count, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_IALLREDUCE);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_IALLREDUCE);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ISCAN);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ISCAN);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iscatter(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        int root, MPIR_Comm * comm,
                                                        MPI_Request * request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ISCATTER);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ISCATTER);
    return MPI_SUCCESS;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_STUBSHM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                                         const int *displs, MPI_Datatype sendtype,
                                                         void *recvbuf, int recvcount,
                                                         MPI_Datatype recvtype, int root,
                                                         MPIR_Comm * comm, MPI_Request * request)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_STUBSHM_MPI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_STUBSHM_MPI_ISCATTERV);

    MPIR_Assert(0);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_STUBSHM_MPI_ISCATTERV);
    return MPI_SUCCESS;
}

#endif /* STUBSHM_COLL_H_INCLUDED */
