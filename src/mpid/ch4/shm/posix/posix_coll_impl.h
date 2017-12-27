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
#define FUNCNAME MPIDI_POSIX_Barrier_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Barrier_recursive_doubling(MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag,
                                           MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier_intra_recursive_doubling(comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_binomial(void *buffer,
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
#define FUNCNAME MPIDI_POSIX_Bcast_scatter_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_scatter_doubling_allgather(void *buffer,
                                                 int count,
                                                 MPI_Datatype datatype,
                                                 int root,
                                                 MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag,
                                                 MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_intra_scatter_doubling_allgather(buffer, count, datatype, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_Bcast_scatter_doubling_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_Bcast_scatter_ring_allgather(void *buffer,
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
#define FUNCNAME MPIDI_POSIX_allreduce_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_allreduce_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                             MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_allreduce_reduce_scatter_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_allreduce_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                   MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_reduce_redscat_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_reduce_redscat_gather(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                      MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_intra_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_reduce_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_reduce_binomial(const void *sendbuf, void *recvbuf, int count,
                                MPI_Datatype datatype, MPI_Op op, int root,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                MPIDI_POSIX_coll_algo_container_t * params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_gather_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_gather_intra_binomial(const void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      int recvcount, MPI_Datatype recvtype,
                                      int root, MPIR_Comm * comm,
                                      MPIR_Errflag_t * errflag,
                                      MPIDI_POSIX_coll_algo_container_t *
                                      params_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_gather_intra_binomial_indexed
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_gather_intra_binomial_indexed(const void *sendbuf, int sendcount,
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
#define FUNCNAME MPIDI_POSIX_gatherv_intra_linear_ssend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_gatherv_intra_linear_ssend(const void *sendbuf, int sendcount,
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
#define FUNCNAME MPIDI_POSIX_gatherv_intra_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_gatherv_intra_linear(const void *sendbuf, int sendcount,
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
#define FUNCNAME MPIDI_POSIX_scatter_intra_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_scatter_intra_binomial(const void *sendbuf, int sendcount,
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
#define FUNCNAME MPIDI_POSIX_scatterv_intra_linear
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
int MPIDI_POSIX_scatterv_intra_linear(const void *sendbuf, const int *sendcounts,
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

#endif /* POSIX_COLL_IMPL_H_INCLUDED */
