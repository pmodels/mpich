/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef NETMOD_AM_FALLBACK_COLL_H_INCLUDED
#define NETMOD_AM_FALLBACK_COLL_H_INCLUDED

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Barrier_impl(comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_bcast(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                                int root, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag)
{
    return MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf,
                                                    MPI_Aint count, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                    MPIR_Errflag_t * errflag)
{
    return MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgather(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_allgatherv(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * displs, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gather(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 int root, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag)
{
    return MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf,
                            recvcount, recvtype, root, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_gatherv(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  const MPI_Aint * recvcounts,
                                                  const MPI_Aint * displs, MPI_Datatype recvtype,
                                                  int root, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    return MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                             recvcounts, displs, recvtype, root, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatter(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype recvtype,
                                                  int root, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    return MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                             recvcount, recvtype, root, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scatterv(const void *sendbuf, const MPI_Aint * sendcounts,
                                                   const MPI_Aint * displs, MPI_Datatype sendtype,
                                                   void *recvbuf, MPI_Aint recvcount,
                                                   MPI_Datatype recvtype, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype,
                              recvbuf, recvcount, recvtype, root, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoall(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallv(const void *sendbuf,
                                                    const MPI_Aint * sendcounts,
                                                    const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                                    void *recvbuf, const MPI_Aint * recvcounts,
                                                    const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                               recvbuf, recvcounts, rdispls, recvtype, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_alltoallw(const void *sendbuf,
                                                    const MPI_Aint sendcounts[],
                                                    const MPI_Aint sdispls[],
                                                    const MPI_Datatype sendtypes[], void *recvbuf,
                                                    const MPI_Aint recvcounts[],
                                                    const MPI_Aint rdispls[],
                                                    const MPI_Datatype recvtypes[],
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                               recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                                 MPI_Datatype datatype, MPI_Op op, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                         const MPI_Aint recvcounts[],
                                                         MPI_Datatype datatype, MPI_Op op,
                                                         MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag)
{
    return MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                               MPI_Aint recvcount,
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag)
{
    return MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                          datatype, op, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    return MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf,
                                                             MPI_Aint sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             MPI_Aint recvcount,
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm_ptr)
{
    return MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, comm_ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_allgatherv(const void *sendbuf,
                                                              MPI_Aint sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const MPI_Aint recvcounts[],
                                                              const MPI_Aint displs[],
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm_ptr)
{
    return MPIR_Neighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcounts, displs, recvtype, comm_ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, MPI_Aint sendcount,
                                                            MPI_Datatype sendtype, void *recvbuf,
                                                            MPI_Aint recvcount,
                                                            MPI_Datatype recvtype,
                                                            MPIR_Comm * comm_ptr)
{
    return MPIR_Neighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, comm_ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoallv(const void *sendbuf,
                                                             const MPI_Aint sendcounts[],
                                                             const MPI_Aint sdispls[],
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             const MPI_Aint recvcounts[],
                                                             const MPI_Aint rdispls[],
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm_ptr)
{
    return MPIR_Neighbor_alltoallv_impl(sendbuf, sendcounts, sdispls,
                                        sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_neighbor_alltoallw(const void *sendbuf,
                                                             const MPI_Aint sendcounts[],
                                                             const MPI_Aint sdispls[],
                                                             const MPI_Datatype sendtypes[],
                                                             void *recvbuf,
                                                             const MPI_Aint recvcounts[],
                                                             const MPI_Aint rdispls[],
                                                             const MPI_Datatype recvtypes[],
                                                             MPIR_Comm * comm_ptr)
{
    return MPIR_Neighbor_alltoallw_impl(sendbuf, sendcounts, sdispls,
                                        sendtypes, recvbuf, recvcounts,
                                        rdispls, recvtypes, comm_ptr);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf,
                                                              MPI_Aint sendcount,
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              MPI_Aint recvcount,
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Request ** req)
{
    return MPIR_Ineighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf,
                                                               MPI_Aint sendcount,
                                                               MPI_Datatype sendtype, void *recvbuf,
                                                               const MPI_Aint recvcounts[],
                                                               const MPI_Aint displs[],
                                                               MPI_Datatype recvtype,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Request ** req)
{
    return MPIR_Ineighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf,
                                                             MPI_Aint sendcount,
                                                             MPI_Datatype sendtype, void *recvbuf,
                                                             MPI_Aint recvcount,
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm_ptr,
                                                             MPIR_Request ** req)
{
    return MPIR_Ineighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf,
                                                              const MPI_Aint sendcounts[],
                                                              const MPI_Aint sdispls[],
                                                              MPI_Datatype sendtype, void *recvbuf,
                                                              const MPI_Aint recvcounts[],
                                                              const MPI_Aint rdispls[],
                                                              MPI_Datatype recvtype,
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Request ** req)
{
    return MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls,
                                         sendtype, recvbuf, recvcounts,
                                         rdispls, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf,
                                                              const MPI_Aint sendcounts[],
                                                              const MPI_Aint sdispls[],
                                                              const MPI_Datatype sendtypes[],
                                                              void *recvbuf,
                                                              const MPI_Aint recvcounts[],
                                                              const MPI_Aint rdispls[],
                                                              const MPI_Datatype recvtypes[],
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Request ** req)
{
    return MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls,
                                         sendtypes, recvbuf, recvcounts,
                                         rdispls, recvtypes, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Ibarrier_impl(comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ibcast(void *buffer, MPI_Aint count,
                                                 MPI_Datatype datatype, int root,
                                                 MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgather(const void *sendbuf, MPI_Aint sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     MPI_Aint recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, MPI_Aint sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const MPI_Aint * recvcounts,
                                                      const MPI_Aint * displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                      MPIR_Request ** req)
{
    return MPIR_Iallgatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcounts, displs, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf,
                                                     MPI_Aint count, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm,
                                                     MPIR_Request ** request)
{
    return MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoall(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Ialltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallv(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                     MPIR_Request ** req)
{
    return MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                recvbuf, recvcounts, rdispls, recvtype, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ialltoallw(const void *sendbuf,
                                                     const MPI_Aint * sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const MPI_Aint * recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igather(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype recvtype,
                                                  int root, MPIR_Comm * comm_ptr,
                                                  MPIR_Request ** req)
{
    return MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf,
                             recvcount, recvtype, root, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_igatherv(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const MPI_Aint * recvcounts,
                                                   const MPI_Aint * displs, MPI_Datatype recvtype,
                                                   int root, MPIR_Comm * comm_ptr,
                                                   MPIR_Request ** req)
{
    return MPIR_Igatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                              recvcounts, displs, recvtype, root, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                                MPI_Aint recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Request ** req)
{
    return MPIR_Ireduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                           datatype, op, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                          const MPI_Aint recvcounts[],
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  int root, MPIR_Comm * comm_ptr,
                                                  MPIR_Request ** req)
{
    return MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    return MPIR_Iscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, req);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatter(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   int root, MPIR_Comm * comm,
                                                   MPIR_Request ** request)
{
    return MPIR_Iscatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, request);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_NM_mpi_iscatterv(const void *sendbuf,
                                                    const MPI_Aint * sendcounts,
                                                    const MPI_Aint * displs, MPI_Datatype sendtype,
                                                    void *recvbuf, MPI_Aint recvcount,
                                                    MPI_Datatype recvtype, int root,
                                                    MPIR_Comm * comm, MPIR_Request ** request)
{
    return MPIR_Iscatterv_impl(sendbuf, sendcounts, displs, sendtype,
                               recvbuf, recvcount, recvtype, root, comm, request);
}

#endif /* NETMOD_AM_FALLBACK_COLL_H_INCLUDED */
