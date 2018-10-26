/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Mellanox Technologies Ltd.
 *  Copyright (C) Mellanox Technologies Ltd. 2016. ALL RIGHTS RESERVED
 */
#ifndef UCX_COLL_H_INCLUDED
#define UCX_COLL_H_INCLUDED

#include "ucx_impl.h"
#ifdef HAVE_LIBHCOLL
#include "../../../common/hcoll/hcoll.h"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                       const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BARRIER);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Barrier(comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                     int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                     const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BCAST);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BCAST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Allgather(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcount, recvtype, comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                          MPIR_Errflag_t * errflag,
                                          const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);

    mpi_errno = MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcounts, displs, recvtype, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      int root, MPIR_Comm * comm_ptr,
                                      MPIR_Errflag_t * errflag,
                                      const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHER);

    mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag,
                                       const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHERV);

    mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag,
                                       const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTER);

    mpi_errno = MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                        const int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm_ptr,
                                        MPIR_Errflag_t * errflag,
                                        const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTERV);

    mpi_errno = MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype,
                                   recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                        const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Alltoall(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                         const int *sdispls, MPI_Datatype sendtype,
                                         void *recvbuf, const int *recvcounts,
                                         const int *rdispls, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                recvcounts, rdispls, recvtype, comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                    recvbuf, recvcounts, rdispls, recvtype, comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                         const int sdispls[], const MPI_Datatype sendtypes[],
                                         void *recvbuf, const int recvcounts[],
                                         const int rdispls[], const MPI_Datatype recvtypes[],
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);

    mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                    recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                      const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE);

#ifdef HAVE_LIBHCOLL
    mpi_errno = hcoll_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
    if (mpi_errno == MPI_SUCCESS)
        goto fn_exit;
#endif
    mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                              const int recvcounts[], MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag,
                                              const void *algo_parameters_container
                                              ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);

    mpi_errno = MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts,
                                         datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                    int recvcount, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                    MPIR_Errflag_t * errflag,
                                                    const void *algo_parameters_container
                                                    ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);

    mpi_errno = MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                               datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Errflag_t * errflag,
                                    const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCAN);

    mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_exscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Errflag_t * errflag,
                                      const void *algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_EXSCAN);

    mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);

    mpi_errno = MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const int recvcounts[], const int displs[],
                                                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Neighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcounts, displs, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Neighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                  const int sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const int recvcounts[],
                                                  const int rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Neighbor_alltoallv_impl(sendbuf, sendcounts, sdispls,
                                             sendtype, recvbuf, recvcounts,
                                             rdispls, recvtype, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const int recvcounts[], const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm_ptr)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Neighbor_alltoallw_impl(sendbuf, sendcounts, sdispls,
                                             sendtypes, recvbuf, recvcounts,
                                             rdispls, recvtypes, comm_ptr);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   int recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);

    mpi_errno = MPIR_Ineighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const int recvcounts[], const int displs[],
                                                    MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Ineighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcounts, displs,
                                               recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Ineighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                   const int sdispls[], MPI_Datatype sendtype,
                                                   void *recvbuf, const int recvcounts[],
                                                   const int rdispls[], MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls,
                                              sendtype, recvbuf, recvcounts,
                                              rdispls, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                   const MPI_Aint sdispls[],
                                                   const MPI_Datatype sendtypes[], void *recvbuf,
                                                   const int recvcounts[], const MPI_Aint rdispls[],
                                                   const MPI_Datatype recvtypes[],
                                                   MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls,
                                              sendtypes, recvbuf, recvcounts,
                                              rdispls, recvtypes, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBARRIER);

    mpi_errno = MPIR_Ibarrier_impl(comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBCAST);

    mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);

    mpi_errno = MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                           MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);

    mpi_errno = MPIR_Iallgatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                      recvcounts, displs, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);

    mpi_errno = MPIR_Iallreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);

    mpi_errno = MPIR_Ialltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcount, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, MPI_Datatype recvtype,
                                          MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);

    mpi_errno = MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                     recvbuf, recvcounts, rdispls, recvtype, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                          const int *sdispls, const MPI_Datatype sendtypes[],
                                          void *recvbuf, const int *recvcounts,
                                          const int *rdispls, const MPI_Datatype recvtypes[],
                                          MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);

    mpi_errno = MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                     recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);

    mpi_errno = MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHER);

    mpi_errno = MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                        MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHERV);

    mpi_errno = MPIR_Igatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                     int recvcount, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm_ptr,
                                                     MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);

    mpi_errno = MPIR_Ireduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                                datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                               const int recvcounts[], MPI_Datatype datatype,
                                               MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);

    mpi_errno = MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts,
                                          datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root,
                                       MPIR_Comm * comm_ptr, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE);

    mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCAN);

    mpi_errno = MPIR_Iscan_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscatter(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTER);

    mpi_errno = MPIR_Iscatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, root, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                         const int *displs, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);

    mpi_errno = MPIR_Iscatterv_impl(sendbuf, sendcounts, displs, sendtype,
                                    recvbuf, recvcount, recvtype, root, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibarrier_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibarrier_sched(MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBARRIER_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBARRIER_SCHED);

    mpi_errno = MPIR_Ibarrier_sched_impl(comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBARRIER_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibcast_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibcast_sched(void *buffer, int count, MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBCAST_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBCAST_SCHED);

    mpi_errno = MPIR_Ibcast_sched_impl(buffer, count, datatype, root, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBCAST_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgather_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgather_sched(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                                MPI_Datatype recvtype, MPIR_Comm * comm,
                                                MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHER_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHER_SCHED);

    mpi_errno = MPIR_Iallgather_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHER_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgatherv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgatherv_sched(const void *sendbuf, int sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const int *recvcounts, const int *displs,
                                                 MPI_Datatype recvtype, MPIR_Comm * comm,
                                                 MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV_SCHED);

    mpi_errno = MPIR_Iallgatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                            displs, recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallreduce_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallreduce_sched(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE_SCHED);

    mpi_errno = MPIR_Iallreduce_sched_impl(sendbuf, recvbuf, count, datatype, op, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLREDUCE_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoall_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoall_sched(const void *sendbuf, int sendcount,
                                               MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                               MPI_Datatype recvtype, MPIR_Comm * comm,
                                               MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALL_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALL_SCHED);

    mpi_errno = MPIR_Ialltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALL_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoallv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoallv_sched(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], MPI_Datatype sendtype,
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], MPI_Datatype recvtype,
                                                MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV_SCHED);

    mpi_errno = MPIR_Ialltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                           recvcounts, rdispls, recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ialltoallw_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ialltoallw_sched(const void *sendbuf, const int sendcounts[],
                                                const int sdispls[], const MPI_Datatype sendtypes[],
                                                void *recvbuf, const int recvcounts[],
                                                const int rdispls[], const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW_SCHED);

    mpi_errno = MPIR_Ialltoallw_sched_impl(sendbuf, sendcounts, sdispls, sendtypes, recvbuf,
                                           recvcounts, rdispls, recvtypes, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iexscan_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iexscan_sched(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IEXSCAN_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IEXSCAN_SCHED);

    mpi_errno = MPIR_Iexscan_sched_impl(sendbuf, recvbuf, count, datatype, op, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IEXSCAN_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igather_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igather_sched(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                             MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHER_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHER_SCHED);

    mpi_errno = MPIR_Igather_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        root, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHER_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igatherv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igatherv_sched(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHERV_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHERV_SCHED);

    mpi_errno = MPIR_Igatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                         recvtype, root, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHERV_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter_block_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter_block_sched(const void *sendbuf, void *recvbuf,
                                                           int recvcount, MPI_Datatype datatype,
                                                           MPI_Op op, MPIR_Comm * comm,
                                                           MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK_SCHED);

    mpi_errno = MPIR_Ireduce_scatter_block_sched_impl(sendbuf, recvbuf, recvcount, datatype, op,
                                                      comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter_sched(const void *sendbuf, void *recvbuf,
                                                     const int recvcounts[], MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_SCHED);

    mpi_errno = MPIR_Ireduce_scatter_sched_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                                s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_sched(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, int root,
                                             MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCHED);

    mpi_errno = MPIR_Ireduce_sched_impl(sendbuf, recvbuf, count, datatype, op, root, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscan_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscan_sched(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                           MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCAN_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCAN_SCHED);

    mpi_errno = MPIR_Iscan_sched_impl(sendbuf, recvbuf, count, datatype, op, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCAN_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscatter_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscatter_sched(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                              MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTER_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTER_SCHED);

    mpi_errno = MPIR_Iscatter_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                         root, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTER_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscatterv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscatterv_sched(const void *sendbuf, const int *sendcounts,
                                               const int *displs, MPI_Datatype sendtype,
                                               void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                               int root, MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCATTERV_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCATTERV_SCHED);

    mpi_errno = MPIR_Iscatterv_sched_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ISCATTERV_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_allgather_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_allgather_sched(const void *sendbuf, int sendcount,
                                                         MPI_Datatype sendtype, void *recvbuf,
                                                         int recvcount, MPI_Datatype recvtype,
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER_SCHED);

    mpi_errno = MPIR_Ineighbor_allgather_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                    recvcount, recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_allgatherv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_allgatherv_sched(const void *sendbuf, int sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          const int recvcounts[],
                                                          const int displs[], MPI_Datatype recvtype,
                                                          MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV_SCHED);

    mpi_errno = MPIR_Ineighbor_allgatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcounts, displs, recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoall_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoall_sched(const void *sendbuf, int sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        int recvcount, MPI_Datatype recvtype,
                                                        MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL_SCHED);

    mpi_errno = MPIR_Ineighbor_alltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                   recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoallv_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoallv_sched(const void *sendbuf,
                                                         const int sendcounts[],
                                                         const int sdispls[], MPI_Datatype sendtype,
                                                         void *recvbuf, const int recvcounts[],
                                                         const int rdispls[], MPI_Datatype recvtype,
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV_SCHED);

    mpi_errno = MPIR_Ineighbor_alltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype, recvbuf,
                                                    recvcounts, rdispls, recvtype, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV_SCHED);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ineighbor_alltoallw_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ineighbor_alltoallw_sched(const void *sendbuf,
                                                         const int sendcounts[],
                                                         const MPI_Aint sdispls[],
                                                         const MPI_Datatype sendtypes[],
                                                         void *recvbuf, const int recvcounts[],
                                                         const MPI_Aint rdispls[],
                                                         const MPI_Datatype recvtypes[],
                                                         MPIR_Comm * comm, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW_SCHED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW_SCHED);

    mpi_errno = MPIR_Ineighbor_alltoallw_sched_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                                    recvbuf, recvcounts, rdispls, recvtypes, comm,
                                                    s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW_SCHED);
    return mpi_errno;
}

#endif /* UCX_COLL_H_INCLUDED */
