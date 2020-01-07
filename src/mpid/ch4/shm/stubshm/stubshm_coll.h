/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef STUBSHM_COLL_H_INCLUDED
#define STUBSHM_COLL_H_INCLUDED

#include "stubshm_impl.h"
#include "ch4_impl.h"

static inline int MPIDI_STUBSHM_mpi_barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                            const void *algo_parameters_container
                                            ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                          int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                          const void *algo_parameters_container ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                              const void *algo_parameters_container
                                              ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_allgather(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag,
                                              const void *algo_parameters_container
                                              ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_allgatherv(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const int *recvcounts, const int *displs,
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag,
                                               const void *algo_parameters_container
                                               ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_gather(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag,
                                           const void *algo_parameters_container
                                           ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_gatherv(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            const int *recvcounts, const int *displs,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag,
                                            const void *algo_parameters_container
                                            ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_scatter(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag,
                                            const void *algo_parameters_container
                                            ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                             const int *displs, MPI_Datatype sendtype,
                                             void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                             int root, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag,
                                             const void *algo_parameters_container
                                             ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_alltoall(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag,
                                             const void *algo_parameters_container
                                             ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                              const int *sdispls, MPI_Datatype sendtype,
                                              void *recvbuf, const int *recvcounts,
                                              const int *rdispls, MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                              const void *algo_parameters_container
                                              ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                              const int sdispls[], const MPI_Datatype sendtypes[],
                                              void *recvbuf, const int recvcounts[],
                                              const int rdispls[], const MPI_Datatype recvtypes[],
                                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                              const void *algo_parameters_container
                                              ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, int root,
                                           MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                           const void *algo_parameters_container
                                           ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                   const int recvcounts[], MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm_ptr,
                                                   MPIR_Errflag_t * errflag,
                                                   const void *algo_parameters_container
                                                   ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                         int recvcount, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         const void *algo_parameters_container
                                                         ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag,
                                           const void *algo_parameters_container
                                           ATTRIBUTE((unused)))
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        const int recvcounts[], const int displs[],
                                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                       const int sdispls[], MPI_Datatype sendtype,
                                                       void *recvbuf, const int recvcounts[],
                                                       const int rdispls[], MPI_Datatype recvtype,
                                                       MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                       const MPI_Aint sdispls[],
                                                       const MPI_Datatype sendtypes[],
                                                       void *recvbuf, const int recvcounts[],
                                                       const MPI_Aint rdispls[],
                                                       const MPI_Datatype recvtypes[],
                                                       MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         const int recvcounts[], const int displs[],
                                                         MPI_Datatype recvtype,
                                                         MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                        const int sdispls[], MPI_Datatype sendtype,
                                                        void *recvbuf, const int recvcounts[],
                                                        const int rdispls[], MPI_Datatype recvtype,
                                                        MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                        const MPI_Aint sdispls[],
                                                        const MPI_Datatype sendtypes[],
                                                        void *recvbuf, const int recvcounts[],
                                                        const MPI_Aint rdispls[],
                                                        const MPI_Datatype recvtypes[],
                                                        MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ibarrier(MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                           int root, MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iallgather(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                const int *recvcounts, const int *displs,
                                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ialltoall(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                              MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                               const int *sdispls, MPI_Datatype sendtype,
                                               void *recvbuf, const int *recvcounts,
                                               const int *rdispls, MPI_Datatype recvtype,
                                               MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                               const int *sdispls, const MPI_Datatype sendtypes[],
                                               void *recvbuf, const int *recvcounts,
                                               const int *rdispls, const MPI_Datatype recvtypes[],
                                               MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                            MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_igather(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                            MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_igatherv(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                             MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                          int recvcount, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                    const int recvcounts[], MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                    MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, int root,
                                            MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPI_Request * req)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iscatter(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             int recvcount, MPI_Datatype recvtype,
                                             int root, MPIR_Comm * comm, MPI_Request * request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

static inline int MPIDI_STUBSHM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                              const int *displs, MPI_Datatype sendtype,
                                              void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, int root,
                                              MPIR_Comm * comm, MPI_Request * request)
{
    MPIR_Assert(0);

    return MPI_SUCCESS;
}

#endif /* STUBSHM_COLL_H_INCLUDED */
