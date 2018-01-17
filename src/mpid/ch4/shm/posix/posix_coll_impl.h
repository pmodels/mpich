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
#ifndef POSIX_COLL_IMPL_H_INCLUDED
#define POSIX_COLL_IMPL_H_INCLUDED

#include "posix_coll_params.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Barrier_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Barrier_intra_recursive_doubling(MPIR_Comm * comm_ptr,
                                               MPIR_Errflag_t * errflag,
                                               MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier_intra_recursive_doubling(comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_intra_binomial(void *buffer,
                                   int count,
                                   MPI_Datatype datatype,
                                   int root,
                                   MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag,
                                   MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_intra_scatter_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_intra_scatter_doubling_allgather(void *buffer,
                                                     int count,
                                                     MPI_Datatype datatype,
                                                     int root,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag,
                                                     MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_intra_scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_intra_scatter_ring_allgather(void *buffer,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 int root,
                                                 MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag,
                                                 MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allreduce_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allreduce_intra_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                 MPI_Datatype datatype, MPI_Op op,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                 MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                       MPI_Datatype datatype, MPI_Op op,
                                                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                       MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_intra_redscat_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_intra_redscat_gather(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                          MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_intra_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_intra_binomial(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                    MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                    MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype,
                                           comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall_intra_scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall_intra_scattered(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                       MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype,
                                              comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall_intra_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall_intra_pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                      MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall_intra_pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                       MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallv_intra_pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallv_intra_pairwise_sendrecv_replace(const void *sendbuf, const int *sendcounts,
                                                        const int *sdispls, MPI_Datatype sendtype,
                                                        void *recvbuf, const int *recvcounts,
                                                        const int *rdispls, MPI_Datatype recvtype,
                                                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtype,
                                                               recvbuf, recvcounts, rdispls, recvtype,
                                                               comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallv_intra_scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallv_intra_scattered(const void *sendbuf, const int *sendcounts,
                                        const int *sdispls, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts,
                                        const int *rdispls, MPI_Datatype recvtype,
                                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls, sendtype,
                                               recvbuf, recvcounts, rdispls, recvtype,
                                               comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallw_intra_pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallw_intra_pairwise_sendrecv_replace(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                                        const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                                                        const int rdispls[], const MPI_Datatype recvtypes[],
                                                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtypes,
                                                               recvbuf, recvcounts, rdispls, recvtypes,
                                                               comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallw_intra_scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallw_intra_scattered(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                        const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                                        const int rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw_intra_scattered(sendbuf, sendcounts, sdispls, sendtypes,
                                               recvbuf, recvcounts, rdispls, recvtypes,
                                               comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgather_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgather_intra_recursive_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                 MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgather_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgather_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                     MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype,
                                            comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgather_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgather_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                   MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype,
                                          comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgatherv_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgatherv_intra_recursive_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                                  void *recvbuf, const int *recvcounts, const int *displs,
                                                  MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                  MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgatherv_intra_brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgatherv_intra_brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, const int *recvcounts, const int *displs,
                                      MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                      MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs,
                                             recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgatherv_intra_ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgatherv_intra_ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, const int *recvcounts, const int *displs,
                                    MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                    MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs,
                                           recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gather_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gather_intra_binomial(const void *sendbuf, int sendcount,
                                    MPI_Datatype sendtype, void *recvbuf,
                                    int recvcount, MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm,
                                    MPIR_Errflag_t * errflag,
                                    MPIDI_POSIX_coll_algo_container_t *
                                    params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype,
                                           root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gather_intra_binomial_indexed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gather_intra_binomial_indexed(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag,
                                            MPIDI_POSIX_coll_algo_container_t *
                                            params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather_intra_binomial_indexed(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype,
                                                   root, comm, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gatherv_intra_linear_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gatherv_intra_linear_ssend(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         const int *recvcounts, const int *displs,
                                         MPI_Datatype recvtype, int root,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         MPIDI_POSIX_coll_algo_container_t *
                                         params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv_intra_linear_ssend(sendbuf, sendcount, sendtype, recvbuf,
                                                recvcounts, displs, recvtype, root,
                                                comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gatherv_intra_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gatherv_intra_linear(const void *sendbuf, int sendcount,
                                   MPI_Datatype sendtype, void *recvbuf,
                                   const int *recvcounts, const int *displs,
                                   MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                   MPIDI_POSIX_coll_algo_container_t *
                                   params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv_intra_linear(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcounts, displs, recvtype, root, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Scatter_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Scatter_intra_binomial(const void *sendbuf, int sendcount,
                                     MPI_Datatype sendtype, void *recvbuf,
                                     int recvcount, MPI_Datatype recvtype,
                                     int root, MPIR_Comm * comm_ptr,
                                     MPIR_Errflag_t * errflag,
                                     MPIDI_POSIX_coll_algo_container_t *
                                     params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Scatterv_intra_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Scatterv_intra_linear(const void *sendbuf, const int *sendcounts,
                                    const int *displs, MPI_Datatype sendtype,
                                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm_ptr,
                                    MPIR_Errflag_t * errflag,
                                    MPIDI_POSIX_coll_algo_container_t *
                                    params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatterv_intra_linear(sendbuf, sendcounts, displs, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_intra_noncomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_intra_noncomm(const void *sendbuf,
                                             void *recvbuf,
                                             const int recvcounts[],
                                             MPI_Datatype datatype,
                                             MPI_Op op, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag,
                                             MPIDI_POSIX_coll_algo_container_t *
                                             params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_intra_noncomm(sendbuf, recvbuf, recvcounts, datatype,
                                                  op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_intra_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_intra_pairwise(const void *sendbuf,
                                              void *recvbuf,
                                              const int recvcounts[],
                                              MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag,
                                              MPIDI_POSIX_coll_algo_container_t *
                                              params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf, recvcounts, datatype,
                                                   op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_intra_recursive_doubling(const void *sendbuf,
                                                        void *recvbuf,
                                                        const int recvcounts[],
                                                        MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag,
                                                        MPIDI_POSIX_coll_algo_container_t *
                                                        params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf, recvcounts, datatype,
                                                             op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_intra_recursive_halving
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_intra_recursive_halving(const void *sendbuf,
                                                       void *recvbuf,
                                                       const int recvcounts[],
                                                       MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag,
                                                       MPIDI_POSIX_coll_algo_container_t *
                                                       params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_intra_recursive_halving(sendbuf, recvbuf, recvcounts, datatype,
                                                            op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block_intra_noncomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block_intra_noncomm(const void *sendbuf,
                                                   void *recvbuf,
                                                   int recvcount,
                                                   MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm_ptr,
                                                   MPIR_Errflag_t * errflag,
                                                   MPIDI_POSIX_coll_algo_container_t *
                                                   params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block_intra_noncomm(sendbuf, recvbuf, recvcount, datatype,
                                                        op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block_intra_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block_intra_pairwise(const void *sendbuf,
                                                    void *recvbuf,
                                                    int recvcount,
                                                    MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                    MPIR_Errflag_t * errflag,
                                                    MPIDI_POSIX_coll_algo_container_t *
                                                    params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf, recvcount, datatype,
                                                         op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block_intra_recursive_doubling(const void *sendbuf,
                                                              void *recvbuf,
                                                              int recvcount,
                                                              MPI_Datatype datatype,
                                                              MPI_Op op, MPIR_Comm * comm_ptr,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_POSIX_coll_algo_container_t *
                                                              params_container
                                                              ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf, recvcount, datatype,
                                                           op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block_intra_recursive_halving
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block_intra_recursive_halving(const void *sendbuf,
                                                             void *recvbuf,
                                                             int recvcount,
                                                             MPI_Datatype datatype,
                                                             MPI_Op op, MPIR_Comm * comm_ptr,
                                                             MPIR_Errflag_t * errflag,
                                                             MPIDI_POSIX_coll_algo_container_t *
                                                             params_container
                                                             ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf, recvcount, datatype, op,
                                                          comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Scan_intra_generic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Scan_intra_generic(const void *sendbuf,
                                   void *recvbuf,
                                   int count,
                                   MPI_Datatype datatype,
                                   MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag,
                                   MPIDI_POSIX_coll_algo_container_t *
                                   params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scan_intra_generic(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Exscan_intra_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Exscan_intra_recursive_doubling(const void *sendbuf,
                                                void *recvbuf,
                                                int count,
                                                MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag,
                                                MPIDI_POSIX_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                             errflag);
    return mpi_errno;
}

#endif /* POSIX_COLL_IMPL_H_INCLUDED */
