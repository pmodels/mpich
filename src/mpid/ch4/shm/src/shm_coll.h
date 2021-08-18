/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef SHM_COLL_H_INCLUDED
#define SHM_COLL_H_INCLUDED

#include <shm.h>
#include "../posix/shm_inline.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_barrier(comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_bcast(void *buffer, MPI_Aint count,
                                                 MPI_Datatype datatype, int root, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_bcast(buffer, count, datatype, root, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allreduce(const void *sendbuf, void *recvbuf,
                                                     MPI_Aint count, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm,
                                                     MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allgather(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     MPI_Aint recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                    recvtype, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_allgatherv(const void *sendbuf, MPI_Aint sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                     displs, recvtype, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scatter(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   int root, MPIR_Comm * comm,
                                                   MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                  recvtype, root, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scatterv(const void *sendbuf,
                                                    const MPI_Aint * sendcounts,
                                                    const MPI_Aint * displs, MPI_Datatype sendtype,
                                                    void *recvbuf, MPI_Aint recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                   recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_gather(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype recvtype,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                 recvtype, root, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_gatherv(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const MPI_Aint * recvcounts,
                                                   const MPI_Aint * displs, MPI_Datatype recvtype,
                                                   int root, MPIR_Comm * comm,
                                                   MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                  displs, recvtype, root, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoall(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoallv(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm,
                                                     MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                    recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_alltoallw(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                    recvcounts, rdispls, recvtypes, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  int root, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const MPI_Aint * recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op,
                                         comm_ptr, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_reduce_scatter_block(const void *sendbuf,
                                                                void *recvbuf, MPI_Aint recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                               op, comm_ptr, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_scan(const void *sendbuf, void *recvbuf,
                                                MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_exscan(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_allgather(const void *sendbuf,
                                                              MPI_Aint sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              MPI_Aint recvcount,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                             recvcount, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_allgatherv(const void *sendbuf,
                                                               MPI_Aint sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               const MPI_Aint * recvcounts,
                                                               const MPI_Aint * displs,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                              recvcounts, displs, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                              const MPI_Aint * sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              MPI_Datatype sendtype,
                                                              void *recvbuf,
                                                              const MPI_Aint * recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                             recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                              const MPI_Aint * sendcounts,
                                                              const MPI_Aint * sdispls,
                                                              const MPI_Datatype * sendtypes,
                                                              void *recvbuf,
                                                              const MPI_Aint * recvcounts,
                                                              const MPI_Aint * rdispls,
                                                              const MPI_Datatype * recvtypes,
                                                              MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                             recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_neighbor_alltoall(const void *sendbuf,
                                                             MPI_Aint sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             MPI_Aint recvcount,
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcount, recvtype, comm);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_allgather(const void *sendbuf,
                                                               MPI_Aint sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               MPI_Aint recvcount,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                              recvcount, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_allgatherv(const void *sendbuf,
                                                                MPI_Aint sendcount,
                                                                MPI_Datatype sendtype,
                                                                void *recvbuf,
                                                                const MPI_Aint * recvcounts,
                                                                const MPI_Aint * displs,
                                                                MPI_Datatype recvtype,
                                                                MPIR_Comm * comm,
                                                                MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                               recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoall(const void *sendbuf,
                                                              MPI_Aint sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              MPI_Aint recvcount,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                             recvcount, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                               const MPI_Aint * sendcounts,
                                                               const MPI_Aint * sdispls,
                                                               MPI_Datatype sendtype,
                                                               void *recvbuf,
                                                               const MPI_Aint * recvcounts,
                                                               const MPI_Aint * rdispls,
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                              recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                               const MPI_Aint * sendcounts,
                                                               const MPI_Aint * sdispls,
                                                               const MPI_Datatype * sendtypes,
                                                               void *recvbuf,
                                                               const MPI_Aint * recvcounts,
                                                               const MPI_Aint * rdispls,
                                                               const MPI_Datatype * recvtypes,
                                                               MPIR_Comm * comm,
                                                               MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                              recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ibarrier(comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ibcast(void *buffer, MPI_Aint count,
                                                  MPI_Datatype datatype, int root, MPIR_Comm * comm,
                                                  MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallgather(const void *sendbuf, MPI_Aint sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      MPI_Aint recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                     recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallgatherv(const void *sendbuf, MPI_Aint sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const MPI_Aint * recvcounts,
                                                       const MPI_Aint * displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                      displs, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iallreduce(const void *sendbuf, void *recvbuf,
                                                      MPI_Aint count, MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm,
                                                      MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoall(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     MPI_Aint recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                    recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoallv(const void *sendbuf,
                                                      const MPI_Aint * sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm,
                                                      MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                     recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ialltoallw(const void *sendbuf,
                                                      const MPI_Aint * sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype sendtypes[], void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype recvtypes[],
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                     recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iexscan(const void *sendbuf, void *recvbuf,
                                                   MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_igather(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_igather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                  recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_igatherv(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const MPI_Aint * recvcounts,
                                                    const MPI_Aint * displs, MPI_Datatype recvtype,
                                                    int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                   displs, recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce_scatter_block(const void *sendbuf,
                                                                 void *recvbuf, MPI_Aint recvcount,
                                                                 MPI_Datatype datatype, MPI_Op op,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                                op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                           const MPI_Aint * recvcounts,
                                                           MPI_Datatype datatype, MPI_Op op,
                                                           MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_ireduce(const void *sendbuf, void *recvbuf,
                                                   MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                   int root, MPIR_Comm * comm_ptr,
                                                   MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscatter(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype recvtype,
                                                    int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, root, comm, req);

    MPIR_FUNC_EXIT;
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_SHM_mpi_iscatterv(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * displs, MPI_Datatype sendtype,
                                                     void *recvbuf, MPI_Aint recvcount,
                                                     MPI_Datatype recvtype, int root,
                                                     MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_ENTER;

    ret = MPIDI_POSIX_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                    recvcount, recvtype, root, comm_ptr, req);

    MPIR_FUNC_EXIT;
    return ret;
}

#endif /* SHM_COLL_H_INCLUDED */
