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
#ifndef POSIX_COLL_H_INCLUDED
#define POSIX_COLL_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "ch4_coll_select.h"
#include "posix_coll_params.h"
#include "posix_coll_select.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_BARRIER);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Barrier_select(comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Barrier_intra_dissemination_id:
            mpi_errno = MPIR_Barrier_intra_dissemination(comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Barrier_impl(comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_BARRIER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_BCAST);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Bcast_select(buffer, count, datatype, root, comm, errflag,
                                 ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Bcast_intra_binomial_id:
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_POSIX_Bcast_intra_scatter_recursive_doubling_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                      root, comm, errflag);
            break;
        case MPIDI_POSIX_Bcast_intra_scatter_ring_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype,
                                                        root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_BCAST);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ALLREDUCE);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                     ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Allreduce_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                        op, comm, errflag);
            break;
        case MPIDI_POSIX_Allreduce_intra_reduce_scatter_allgather_id:
            mpi_errno =
                MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count,
                                                              datatype, op, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ALLREDUCE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_allgather(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ALLGATHER);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Allgather_select(sendbuf, sendcount, sendtype,
                                     recvbuf, recvcount, recvtype,
                                     comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Allgather_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag);
            break;
        case MPIDI_POSIX_Allgather_intra_brucks_id:
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Allgather_intra_ring_id:
            mpi_errno =
                MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ALLGATHER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_allgatherv(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag,
                                             const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ALLGATHERV);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Allgatherv_select(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs,
                                      recvtype, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Allgatherv_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Allgatherv_intra_brucks_id:
            mpi_errno =
                MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Allgatherv_intra_ring_id:
            mpi_errno =
                MPIR_Allgatherv_intra_ring(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ALLGATHERV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_GATHER);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Gather_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                  recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Gather_intra_binomial_id:
            mpi_errno =
                MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                         recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_GATHER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_GATHERV);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Gatherv_select(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, root,
                                   comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Gatherv_allcomm_linear_id:
            mpi_errno =
                MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                          displs, recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_GATHERV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SCATTER);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Scatter_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                   recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Scatter_intra_binomial_id:
            mpi_errno =
                MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SCATTER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                           const int *displs, MPI_Datatype sendtype,
                                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                           const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SCATTERV);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Scatterv_select(sendbuf, sendcounts, displs, sendtype,
                                    recvbuf, recvcount, recvtype, root,
                                    comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Scatterv_allcomm_linear_id:
            mpi_errno =
                MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype,
                                             recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SCATTERV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_alltoall(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag,
                                           const void *ch4_algo_parameters_container_in)
{
    int mpi_errno;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALL);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Alltoall_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                    recvtype, comm, errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Alltoall_intra_brucks_id:
            mpi_errno =
                MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoall_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoall_intra_pairwise_id:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoall_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise_sendrecv_replace(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALL);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                            const int *sdispls, MPI_Datatype sendtype,
                                            void *recvbuf, const int *recvcounts,
                                            const int *rdispls, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALLV);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Alltoallv_select(sendbuf, sendcounts, sdispls,
                                     sendtype, recvbuf, recvcounts,
                                     rdispls, recvtype, comm,
                                     errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Alltoallv_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtype, recvbuf, recvcounts,
                                                               rdispls, recvtype, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoallv_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoallv_intra_scattered(sendbuf, sendcounts, sdispls,
                                               sendtype, recvbuf, recvcounts,
                                               rdispls, recvtype, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls,
                                            sendtype, recvbuf, recvcounts,
                                            rdispls, recvtype, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALLV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                            const void *ch4_algo_parameters_container_in)
{
    int mpi_errno;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALLW);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Alltoallw_select(sendbuf, sendcounts, sdispls,
                                     sendtypes, recvbuf, recvcounts,
                                     rdispls, recvtypes, comm,
                                     errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Alltoallw_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtypes, recvbuf,
                                                               recvcounts, rdispls,
                                                               recvtypes, comm, errflag);
            break;
        case MPIDI_POSIX_Alltoallw_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoallw_intra_scattered(sendbuf, sendcounts, sdispls,
                                               sendtypes, recvbuf, recvcounts,
                                               rdispls, recvtypes, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls,
                                            sendtypes, recvbuf, recvcounts,
                                            rdispls, recvtypes, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ALLTOALLW);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, int root,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_REDUCE);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Reduce_intra_reduce_scatter_gather_id:
            mpi_errno =
                MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count, datatype,
                                                        op, root, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_intra_binomial_id:
            mpi_errno =
                MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count, datatype,
                                           op, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op,
                                         root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_REDUCE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                                 const int recvcounts[], MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag,
                                                 const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_REDUCE_SCATTER);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Reduce_scatter_select(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                          errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Reduce_scatter_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_noncommutative(sendbuf, recvbuf, recvcounts,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf, recvcounts,
                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_intra_recursive_halving_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                            datatype, op, comm, errflag);
            break;
        default:
            mpi_errno =
                MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_REDUCE_SCATTER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm,
                                                       MPIR_Errflag_t * errflag,
                                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_REDUCE_SCATTER_BLOCK);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Reduce_scatter_block_select(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                                errflag, ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Reduce_scatter_block_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_noncommutative(sendbuf, recvbuf, recvcount,
                                                               datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_block_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf, recvcount,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_block_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf,
                                                                   recvcount, datatype, op,
                                                                   comm, errflag);
            break;
        case MPIDI_POSIX_Reduce_scatter_block_intra_recursive_halving_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf,
                                                                  recvcount, datatype, op,
                                                                  comm, errflag);
            break;
        default:
            mpi_errno =
                MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                               errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_REDUCE_SCATTER_BLOCK);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_SCAN);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Scan_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Scan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                   op, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_SCAN);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_exscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_POSIX_coll_algo_container_t *shm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_EXSCAN);

    shm_algo_parameters_container_out =
        MPIDI_POSIX_Exscan_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (shm_algo_parameters_container_out->id) {
        case MPIDI_POSIX_Exscan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype,
                                                     op, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_EXSCAN);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLGATHER);

    mpi_errno =
        MPIR_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_neighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int recvcounts[], const int displs[],
                                                      MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_neighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_neighbor_alltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Neighbor_alltoall(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_neighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_neighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                     const int sdispls[], MPI_Datatype sendtype,
                                                     void *recvbuf, const int recvcounts[],
                                                     const int rdispls[], MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                        recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_neighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_neighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                     const MPI_Aint sdispls[],
                                                     const MPI_Datatype sendtypes[], void *recvbuf,
                                                     const int recvcounts[],
                                                     const MPI_Aint rdispls[],
                                                     const MPI_Datatype recvtypes[],
                                                     MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                        recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_NEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ineighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ineighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLGATHER);

    mpi_errno = MPIR_Ineighbor_allgather(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ineighbor_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int recvcounts[], const int displs[],
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ineighbor_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ineighbor_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ineighbor_alltoallv(const void *sendbuf, const int sendcounts[],
                                                      const int sdispls[], MPI_Datatype sendtype,
                                                      void *recvbuf, const int recvcounts[],
                                                      const int rdispls[], MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                         recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ineighbor_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ineighbor_alltoallw(const void *sendbuf, const int sendcounts[],
                                                      const MPI_Aint sdispls[],
                                                      const MPI_Datatype sendtypes[], void *recvbuf,
                                                      const int recvcounts[],
                                                      const MPI_Aint rdispls[],
                                                      const MPI_Datatype recvtypes[],
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                         recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_INEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IBARRIER);

    mpi_errno = MPIR_Ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IBARRIER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IBCAST);

    mpi_errno = MPIR_Ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IBCAST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iallgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IALLGATHER);

    mpi_errno = MPIR_Iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IALLGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iallgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iallgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IALLGATHERV);

    mpi_errno = MPIR_Iallgatherv(sendbuf, sendcount, sendtype,
                                 recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IALLGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ialltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ialltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALL);

    mpi_errno = MPIR_Ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALL);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ialltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ialltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALLV);

    mpi_errno = MPIR_Ialltoallv(sendbuf, sendcounts, sdispls,
                                sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALLV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ialltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ialltoallw(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALLW);

    mpi_errno = MPIR_Ialltoallw(sendbuf, sendcounts, sdispls,
                                sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IEXSCAN);

    mpi_errno = MPIR_Iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IEXSCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IGATHER);

    mpi_errno = MPIR_Igather(sendbuf, sendcount, sendtype, recvbuf,
                             recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_igatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IGATHERV);

    mpi_errno = MPIR_Igatherv(sendbuf, sendcount, sendtype,
                              recvbuf, recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm,
                                                        MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE_SCATTER_BLOCK);

    mpi_errno = MPIR_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE_SCATTER_BLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ireduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE_SCATTER);

    mpi_errno = MPIR_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE);

    mpi_errno = MPIR_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iallreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iallreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_IALLREDUCE);

    mpi_errno = MPIR_Iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IALLREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ISCAN);

    mpi_errno = MPIR_Iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ISCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iscatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iscatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           int recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ISCATTER);

    mpi_errno = MPIR_Iscatter(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ISCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_POSIX_mpi_iscatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_POSIX_mpi_iscatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, int root,
                                            MPIR_Comm * comm, MPIR_Request ** request)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_MPI_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_MPI_ISCATTERV);

    mpi_errno = MPIR_Iscatterv(sendbuf, sendcounts, displs, sendtype,
                               recvbuf, recvcount, recvtype, root, comm, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_ISCATTERV);
    return mpi_errno;
}

#endif /* POSIX_COLL_H_INCLUDED */
