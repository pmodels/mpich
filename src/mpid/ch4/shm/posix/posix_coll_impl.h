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
#define FUNCNAME MPIDI_POSIX_Barrier__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Barrier__intra__dissemination(MPIR_Comm * comm_ptr,
                                              MPIR_Errflag_t * errflag,
                                              MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier__intra__dissemination(comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast__intra__binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast__intra__binomial(void *buffer,
                                       int count,
                                       MPI_Datatype datatype,
                                       int root,
                                       MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag,
                                       MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast__intra__binomial(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast__intra__scatter_recursive_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast__intra__scatter_recursive_doubling_allgather(void *buffer,
                                                                   int count,
                                                                   MPI_Datatype datatype,
                                                                   int root,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag,
                                                                   MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast__intra__scatter_recursive_doubling_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast__intra__scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast__intra__scatter_ring_allgather(void *buffer,
                                                     int count,
                                                     MPI_Datatype datatype,
                                                     int root,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag,
                                                     MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast__intra__scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allreduce__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allreduce__intra__recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op,
                                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                     MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce__intra__recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allreduce__intra__reduce_scatter_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allreduce__intra__reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                           MPI_Datatype datatype, MPI_Op op,
                                                           MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                           MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce__intra__reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce__intra__redscat_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce__intra__reduce_scatter_gather(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op, int root,
                                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                     MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce__intra__reduce_scatter_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce__intra__binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce__intra__binomial(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, int root,
                                        MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce__intra__binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall__intra__brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall__intra__brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall__intra__brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall__intra__scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall__intra__scattered(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                           MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                           MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall__intra__scattered(sendbuf, sendcount, sendtype,
                                                recvbuf, recvcount, recvtype,
                                                comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall__intra__pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall__intra__pairwise(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                          MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall__intra__pairwise(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcount, recvtype,
                                               comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoall__intra__pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoall__intra__pairwise_sendrecv_replace(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                                           MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                           MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall__intra__pairwise_sendrecv_replace(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype,
                                                                comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallv__intra__pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallv__intra__pairwise_sendrecv_replace(const void *sendbuf, const int *sendcounts,
                                                            const int *sdispls, MPI_Datatype sendtype,
                                                            void *recvbuf, const int *recvcounts,
                                                            const int *rdispls, MPI_Datatype recvtype,
                                                            MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                            MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv__intra__pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtype,
                                                                 recvbuf, recvcounts, rdispls, recvtype,
                                                                 comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallv__intra__scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallv__intra__scattered(const void *sendbuf, const int *sendcounts,
                                            const int *sdispls, MPI_Datatype sendtype,
                                            void *recvbuf, const int *recvcounts,
                                            const int *rdispls, MPI_Datatype recvtype,
                                            MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                            MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv__intra__scattered(sendbuf, sendcounts, sdispls, sendtype,
                                                 recvbuf, recvcounts, rdispls, recvtype,
                                                 comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallw__intra__pairwise_sendrecv_replace
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallw__intra__pairwise_sendrecv_replace(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                                            const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                                            MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                            MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw__intra__pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls, sendtypes,
                                                                 recvbuf, recvcounts, rdispls, recvtypes,
                                                                 comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Alltoallw__intra__scattered
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Alltoallw__intra__scattered(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                            const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                            MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw__intra__scattered(sendbuf, sendcounts, sdispls, sendtypes,
                                                 recvbuf, recvcounts, rdispls, recvtypes,
                                                 comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgather__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgather__intra__recursive_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                                     void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                     MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather__intra__recursive_doubling(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcount, recvtype,
                                                          comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgather__intra__brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgather__intra__brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                         MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather__intra__brucks(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype,
                                              comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgather__intra__ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgather__intra__ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                       MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather__intra__ring(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype,
                                            comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgatherv__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgatherv__intra__recursive_doubling(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                                      MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv__intra__recursive_doubling(sendbuf, sendcount, sendtype,
                                                           recvbuf, recvcounts, displs,
                                                           recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgatherv__intra__brucks
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgatherv__intra__brucks(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                          MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv__intra__brucks(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcounts, displs,
                                               recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Allgatherv__intra__ring
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Allgatherv__intra__ring(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                        MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv__intra__ring(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs,
                                             recvtype, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gather__intra__binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gather__intra__binomial(const void *sendbuf, int sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag,
                                        MPIDI_POSIX_coll_algo_container_t *
                                        params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather__intra__binomial(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype,
                                             root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gather__intra__binomial_indexed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gather__intra__binomial_indexed(const void *sendbuf, int sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                int recvcount, MPI_Datatype recvtype,
                                                int root, MPIR_Comm * comm,
                                                MPIR_Errflag_t * errflag,
                                                MPIDI_POSIX_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather__intra__binomial_indexed(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype,
                                                     root, comm, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gatherv__intra__linear_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gatherv__intra__linear_ssend(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, int root,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                             MPIDI_POSIX_coll_algo_container_t *
                                             params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv__intra__linear_ssend(sendbuf, sendcount, sendtype, recvbuf,
                                                  recvcounts, displs, recvtype, root,
                                                  comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Gatherv__intra__linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Gatherv__intra__linear(const void *sendbuf, int sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, int root,
                                       MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                       MPIDI_POSIX_coll_algo_container_t *
                                       params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv__intra__linear(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, root, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Scatter__intra__binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Scatter__intra__binomial(const void *sendbuf, int sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         int recvcount, MPI_Datatype recvtype,
                                         int root, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag,
                                         MPIDI_POSIX_coll_algo_container_t *
                                         params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatter__intra__binomial(sendbuf, sendcount, sendtype, recvbuf,
                                              recvcount, recvtype, root, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Scatterv__intra__linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Scatterv__intra__linear(const void *sendbuf, const int *sendcounts,
                                        const int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm_ptr,
                                        MPIR_Errflag_t * errflag,
                                        MPIDI_POSIX_coll_algo_container_t *
                                        params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatterv__intra__linear(sendbuf, sendcounts, displs, sendtype,
                                             recvbuf, recvcount, recvtype, root, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter__intra__noncommutative
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter__intra__noncommutative(const void *sendbuf,
                                                      void *recvbuf,
                                                      const int recvcounts[],
                                                      MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag,
                                                      MPIDI_POSIX_coll_algo_container_t *
                                                      params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter__intra__noncommutative(sendbuf, recvbuf, recvcounts, datatype,
                                                           op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter__intra__pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter__intra__pairwise(const void *sendbuf,
                                                void *recvbuf,
                                                const int recvcounts[],
                                                MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag,
                                                MPIDI_POSIX_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter__intra__pairwise(sendbuf, recvbuf, recvcounts, datatype,
                                                     op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter__intra__recursive_doubling(const void *sendbuf,
                                                          void *recvbuf,
                                                          const int recvcounts[],
                                                          MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag,
                                                          MPIDI_POSIX_coll_algo_container_t *
                                                          params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter__intra__recursive_doubling(sendbuf, recvbuf, recvcounts, datatype,
                                                               op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter__intra__recursive_halving
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter__intra__recursive_halving(const void *sendbuf,
                                                         void *recvbuf,
                                                         const int recvcounts[],
                                                         MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm_ptr,
                                                         MPIR_Errflag_t * errflag,
                                                         MPIDI_POSIX_coll_algo_container_t *
                                                         params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter__intra__recursive_halving(sendbuf, recvbuf, recvcounts, datatype,
                                                              op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block__intra__noncommutative
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block__intra__noncommutative(const void *sendbuf,
                                                            void *recvbuf,
                                                            int recvcount,
                                                            MPI_Datatype datatype,
                                                            MPI_Op op, MPIR_Comm * comm_ptr,
                                                            MPIR_Errflag_t * errflag,
                                                            MPIDI_POSIX_coll_algo_container_t *
                                                            params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block__intra__noncommutative(sendbuf, recvbuf, recvcount, datatype,
                                                                 op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block__intra__pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block__intra__pairwise(const void *sendbuf,
                                                      void *recvbuf,
                                                      int recvcount,
                                                      MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag,
                                                      MPIDI_POSIX_coll_algo_container_t *
                                                      params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block__intra__pairwise(sendbuf, recvbuf, recvcount, datatype,
                                                           op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block__intra__recursive_doubling(const void *sendbuf,
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
        MPIR_Reduce_scatter_block__intra__recursive_doubling(sendbuf, recvbuf, recvcount, datatype,
                                                             op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Reduce_scatter_block__intra__recursive_halving
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Reduce_scatter_block__intra__recursive_halving(const void *sendbuf,
                                                               void *recvbuf,
                                                               int recvcount,
                                                               MPI_Datatype datatype,
                                                               MPI_Op op, MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_POSIX_coll_algo_container_t *
                                                               params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Reduce_scatter_block__intra__recursive_halving(sendbuf, recvbuf, recvcount, datatype, op,
                                                            comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Scan__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Scan__intra__recursive_doubling(const void *sendbuf,
                                                void *recvbuf,
                                                int count,
                                                MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag,
                                                MPIDI_POSIX_coll_algo_container_t *
                                                params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scan__intra__recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Exscan__intra__recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Exscan__intra__recursive_doubling(const void *sendbuf,
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
        MPIR_Exscan__intra__recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                               errflag);
    return mpi_errno;
}
#endif /* POSIX_COLL_IMPL_H_INCLUDED */
