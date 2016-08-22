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
#ifndef SHM_STUBSHM_COLL_H_INCLUDED
#define SHM_STUBSHM_COLL_H_INCLUDED

#include "stubshm_impl.h"
#include "ch4_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_bcast(void *buffer, int count, MPI_Datatype datatype,
                                  int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_allreduce(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const int *recvcounts, const int *displs,
                                    MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_scatterv(const void *sendbuf, const int *sendcounts,
                                     const int *displs, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_alltoallv(const void *sendbuf, const int *sendcounts,
                                      const int *sdispls, MPI_Datatype sendtype,
                                      void *recvbuf, const int *recvcounts,
                                      const int *rdispls, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_alltoallw(const void *sendbuf, const int sendcounts[],
                                      const int sdispls[], const MPI_Datatype sendtypes[],
                                      void *recvbuf, const int recvcounts[],
                                      const int rdispls[], const MPI_Datatype recvtypes[],
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_reduce(const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_reduce_scatter(const void *sendbuf, void *recvbuf,
                                           const int recvcounts[], MPI_Datatype datatype,
                                           MPI_Op op, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                 int recvcount, MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_scan(const void *sendbuf, void *recvbuf, int count,
                                 MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_exscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_exscan(const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_neighbor_allgather(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_neighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                const int recvcounts[], const int displs[],
                                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_neighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_neighbor_alltoall(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_neighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                               const int sdispls[], MPI_Datatype sendtype,
                                               void *recvbuf, const int recvcounts[],
                                               const int rdispls[], MPI_Datatype recvtype,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_neighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                               const MPI_Aint sdispls[],
                                               const MPI_Datatype sendtypes[], void *recvbuf,
                                               const int recvcounts[], const MPI_Aint rdispls[],
                                               const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ineighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ineighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const int recvcounts[], const int displs[],
                                                 MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                                 MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ineighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               int recvcount, MPI_Datatype recvtype,
                                               MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ineighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], MPI_Datatype sendtype,
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], MPI_Datatype recvtype,
                                                MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ineighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                const MPI_Aint sdispls[],
                                                const MPI_Datatype sendtypes[], void *recvbuf,
                                                const int recvcounts[], const MPI_Aint rdispls[],
                                                const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ibarrier(MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                   int root, MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ialltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ialltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ialltoallv(const void *sendbuf, const int *sendcounts,
                                       const int *sdispls, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts,
                                       const int *rdispls, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ialltoallw(const void *sendbuf, const int *sendcounts,
                                       const int *sdispls, const MPI_Datatype sendtypes[],
                                       void *recvbuf, const int *recvcounts,
                                       const int *rdispls, const MPI_Datatype recvtypes[],
                                       MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iexscan(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, const int *recvcounts, const int *displs,
                                     MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                     MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                  int recvcount, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ireduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                            const int recvcounts[], MPI_Datatype datatype,
                                            MPI_Op op, MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_ireduce(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op,
                                       MPIR_Comm * comm_ptr, MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iscan(const void *sendbuf, void *recvbuf, int count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  MPI_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(0);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iscatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iscatter(const void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int recvcount, MPI_Datatype recvtype,
                                     int root, MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_SHM_iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_SHM_iscatterv(const void *sendbuf, const int *sendcounts,
                                      const int *displs, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount,
                                      MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Assert(0);
    return mpi_errno;
}

#endif /* SHM_STUBSHM_COLL_H_INCLUDED */
