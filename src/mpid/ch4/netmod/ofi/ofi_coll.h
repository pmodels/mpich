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
#ifndef OFI_COLL_H_INCLUDED
#define OFI_COLL_H_INCLUDED

#include "ofi_impl.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"
#include "ofi_coll_select.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_barrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BARRIER);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Barrier_select(comm, errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Barrier_intra_dissemination_id:
            mpi_errno = MPIR_Barrier_intra_dissemination(comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Barrier_impl(comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BARRIER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_bcast(void *buffer, int count, MPI_Datatype datatype,
                                     int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                     const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_BCAST);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Bcast_select(buffer, count, datatype, root, comm, errflag,
                               ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Bcast_intra_binomial_id:
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_id:
            mpi_errno =
                MPIR_Bcast_intra_scatter_recursive_doubling_allgather(buffer, count, datatype,
                                                                      root, comm, errflag);
            break;
        case MPIDI_OFI_Bcast_intra_scatter_ring_allgather_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_BCAST);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Allreduce_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count,
                                                        datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLREDUCE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Allgather_select(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcount, recvtype, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Allgather_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag);
            break;
        case MPIDI_OFI_Allgather_intra_brucks_id:
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Allgather_intra_ring_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_allgatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag,
                                          const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Allgatherv_select(sendbuf, sendcount, sendtype,
                                    recvbuf, recvcounts, displs, recvtype, comm, errflag,
                                    ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Allgatherv_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Allgatherv_intra_recursive_doubling(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Allgatherv_intra_brucks_id:
            mpi_errno =
                MPIR_Allgatherv_intra_brucks(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcounts, displs, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Allgatherv_intra_ring_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLGATHERV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_gather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                      void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                      int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                      const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHER);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Gather_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Gather_intra_binomial_id:
            mpi_errno =
                MPIR_Gather_intra_binomial(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_gatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, const int *recvcounts, const int *displs,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_GATHERV);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Gatherv_select(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                 displs, recvtype, root, comm, errflag,
                                 ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Gatherv_allcomm_linear_id:
            mpi_errno =
                MPIR_Gatherv_allcomm_linear(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcounts, displs, recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_GATHERV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                       const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTER);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Scatter_select(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                 recvtype, root, comm, errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Scatter_intra_binomial_id:
            mpi_errno =
                MPIR_Scatter_intra_binomial(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scatterv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scatterv(const void *sendbuf, const int *sendcounts,
                                        const int *displs, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCATTERV);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Scatterv_select(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Scatterv_allcomm_linear_id:
            mpi_errno =
                MPIR_Scatterv_allcomm_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                             recvcount, recvtype, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                           recvcount, recvtype, root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCATTERV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoall
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                        const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Alltoall_select(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, errflag,
                                  ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Alltoall_intra_brucks_id:
            mpi_errno =
                MPIR_Alltoall_intra_brucks(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoall_intra_scattered_id:
            mpi_errno =
                MPIR_Alltoall_intra_scattered(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoall_intra_pairwise_id:
            mpi_errno =
                MPIR_Alltoall_intra_pairwise(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoall_intra_pairwise_sendrecv_replace_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALL);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoallv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoallv(const void *sendbuf, const int *sendcounts,
                                         const int *sdispls, MPI_Datatype sendtype,
                                         void *recvbuf, const int *recvcounts,
                                         const int *rdispls, MPI_Datatype recvtype,
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Alltoallv_select(sendbuf, sendcounts, sdispls,
                                   sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Alltoallv_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallv_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtype, recvbuf, recvcounts,
                                                               rdispls, recvtype, comm, errflag);
            break;
        case MPIDI_OFI_Alltoallv_intra_scattered_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLV);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_alltoallw
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_alltoallw(const void *sendbuf, const int sendcounts[],
                                         const int sdispls[], const MPI_Datatype sendtypes[],
                                         void *recvbuf, const int recvcounts[],
                                         const int rdispls[], const MPI_Datatype recvtypes[],
                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                         const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Alltoallw_select(sendbuf, sendcounts, sdispls,
                                   sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes, comm, errflag,
                                   ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Alltoallw_intra_pairwise_sendrecv_replace_id:
            mpi_errno =
                MPIR_Alltoallw_intra_pairwise_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                               sendtypes, recvbuf, recvcounts,
                                                               rdispls, recvtypes, comm, errflag);
            break;
        case MPIDI_OFI_Alltoallw_intra_scattered_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_ALLTOALLW);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                      const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Reduce_intra_reduce_scatter_gather_id:
            mpi_errno =
                MPIR_Reduce_intra_reduce_scatter_gather(sendbuf, recvbuf, count,
                                                        datatype, op, root, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_intra_binomial_id:
            mpi_errno =
                MPIR_Reduce_intra_binomial(sendbuf, recvbuf, count,
                                           datatype, op, root, comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op,
                                         root, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce_scatter(const void *sendbuf, void *recvbuf,
                                              const int recvcounts[], MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag,
                                              const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Reduce_scatter_select(sendbuf, recvbuf, recvcounts, datatype, op, comm,
                                        errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Reduce_scatter_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_noncommutative(sendbuf, recvbuf, recvcounts,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_pairwise(sendbuf, recvbuf, recvcounts,
                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_intra_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_intra_recursive_halving_id:
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

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_reduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                    int recvcount, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm,
                                                    MPIR_Errflag_t * errflag,
                                                    const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Reduce_scatter_block_select(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                              errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Reduce_scatter_block_intra_noncommutative_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_noncommutative(sendbuf, recvbuf, recvcount,
                                                               datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_block_intra_pairwise_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_pairwise(sendbuf, recvbuf, recvcount,
                                                         datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_block_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_doubling(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_OFI_Reduce_scatter_block_intra_recursive_halving_id:
            mpi_errno =
                MPIR_Reduce_scatter_block_intra_recursive_halving(sendbuf, recvbuf, recvcount,
                                                                  datatype, op, comm, errflag);
            break;
        default:
            mpi_errno =
                MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                               errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_REDUCE_SCATTER_BLOCK);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_scan(const void *sendbuf, void *recvbuf, int count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                    MPIR_Errflag_t * errflag,
                                    const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_SCAN);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Scan_select(sendbuf, recvbuf, count, datatype, op, comm,
                              errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Scan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Scan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                   comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_SCAN);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_exscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_exscan(const void *sendbuf, void *recvbuf, int count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                      MPIR_Errflag_t * errflag,
                                      const void *ch4_algo_parameters_container_in)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_OFI_coll_algo_container_t *nm_algo_parameters_container_out = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_EXSCAN);

    nm_algo_parameters_container_out =
        MPIDI_OFI_Exscan_select(sendbuf, recvbuf, count, datatype, op, comm,
                                errflag, ch4_algo_parameters_container_in);

    switch (nm_algo_parameters_container_out->id) {
        case MPIDI_OFI_Exscan_intra_recursive_doubling_id:
            mpi_errno =
                MPIR_Exscan_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                     comm, errflag);
            break;
        default:
            mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
            break;
    }

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_EXSCAN);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_neighbor_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_neighbor_allgather(const void *sendbuf, int sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  int recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHER);

    mpi_errno =
        MPIR_Neighbor_allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                     comm);

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
                                                   MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Neighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcounts, displs, recvtype, comm);

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
                                                 MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Neighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcount, recvtype, comm);

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
                                                  MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Neighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                             recvbuf, recvcounts, rdispls, recvtype, comm);

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
                                                  const MPI_Datatype recvtypes[], MPIR_Comm * comm)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_NEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Neighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                             recvbuf, recvcounts, rdispls, recvtypes, comm);

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
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHER);

    mpi_errno = MPIR_Ineighbor_allgather_impl(sendbuf, sendcount, sendtype,
                                              recvbuf, recvcount, recvtype, comm, req);

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
                                                    MPI_Datatype recvtype, MPIR_Comm * comm,
                                                    MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLGATHERV);

    mpi_errno = MPIR_Ineighbor_allgatherv_impl(sendbuf, sendcount, sendtype,
                                               recvbuf, recvcounts, displs, recvtype, comm, req);

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
                                                  MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALL);

    mpi_errno = MPIR_Ineighbor_alltoall_impl(sendbuf, sendcount, sendtype,
                                             recvbuf, recvcount, recvtype, comm, req);

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
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLV);

    mpi_errno = MPIR_Ineighbor_alltoallv_impl(sendbuf, sendcounts, sdispls, sendtype,
                                              recvbuf, recvcounts, rdispls, recvtype, comm, req);

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
                                                   MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);

    mpi_errno = MPIR_Ineighbor_alltoallw_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                              recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBARRIER);

    mpi_errno = MPIR_Ibarrier_impl(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBARRIER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ibcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ibcast(void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IBCAST);

    mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IBCAST);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iallgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHER);

    mpi_errno = MPIR_Iallgather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                     recvcount, recvtype, comm, req);

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
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLGATHERV);

    mpi_errno = MPIR_Iallgatherv_impl(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs, recvtype, comm, req);

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
                                         MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALL);

    mpi_errno = MPIR_Ialltoall_impl(sendbuf, sendcount, sendtype, recvbuf,
                                    recvcount, recvtype, comm, req);

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
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLV);

    mpi_errno = MPIR_Ialltoallv_impl(sendbuf, sendcounts, sdispls,
                                     sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, req);

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
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);

    mpi_errno = MPIR_Ialltoallw_impl(sendbuf, sendcounts, sdispls,
                                     sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IALLTOALLW);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iexscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iexscan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);

    mpi_errno = MPIR_Iexscan_impl(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IEXSCAN);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                       void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                       int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHER);

    mpi_errno = MPIR_Igather_impl(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_igatherv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                        void *recvbuf, const int *recvcounts, const int *displs,
                                        MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IGATHERV);

    mpi_errno = MPIR_Igatherv_impl(sendbuf, sendcount, sendtype,
                                   recvbuf, recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IGATHERV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                     int recvcount, MPI_Datatype datatype,
                                                     MPI_Op op, MPIR_Comm * comm,
                                                     MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);

    mpi_errno = MPIR_Ireduce_scatter_block_impl(sendbuf, recvbuf, recvcount,
                                                datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER_BLOCK);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce_scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce_scatter(const void *sendbuf, void *recvbuf,
                                               const int recvcounts[], MPI_Datatype datatype,
                                               MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);

    mpi_errno = MPIR_Ireduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE_SCATTER);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_ireduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_ireduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root,
                                       MPIR_Comm * comm, MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_IREDUCE);

    mpi_errno = MPIR_Ireduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_IREDUCE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_NM_mpi_iscan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_NM_mpi_iscan(const void *sendbuf, void *recvbuf, int count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                     MPIR_Request ** req)
{
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_NM_MPI_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_NM_MPI_ISCAN);

    mpi_errno = MPIR_Iscan_impl(sendbuf, recvbuf, count, datatype, op, comm, req);

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

    mpi_errno = MPIR_Iallgather_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                           recvcount, recvtype, comm, s);

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

    mpi_errno = MPIR_Iallgatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                            recvcounts, displs, recvtype, comm, s);

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

    mpi_errno = MPIR_Ialltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, comm, s);

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

    mpi_errno = MPIR_Ialltoallv_sched_impl(sendbuf, sendcounts, sdispls, sendtype,
                                           recvbuf, recvcounts, rdispls, recvtype, comm, s);

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

    mpi_errno = MPIR_Ialltoallw_sched_impl(sendbuf, sendcounts, sdispls, sendtypes,
                                           recvbuf, recvcounts, rdispls, recvtypes, comm, s);

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

    mpi_errno = MPIR_Igather_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                        recvtype, root, comm, s);

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

    mpi_errno = MPIR_Igatherv_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                         displs, recvtype, root, comm, s);

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

    mpi_errno = MPIR_Ireduce_scatter_block_sched_impl(sendbuf, recvbuf, recvcount,
                                                      datatype, op, comm, s);

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

    mpi_errno = MPIR_Ireduce_scatter_sched_impl(sendbuf, recvbuf, recvcounts, datatype,
                                                op, comm, s);

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

    mpi_errno = MPIR_Iscatter_sched_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                         recvtype, root, comm, s);

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

    mpi_errno = MPIR_Iscatterv_sched_impl(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                          recvcount, recvtype, root, comm, s);

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

    mpi_errno = MPIR_Ineighbor_allgatherv_sched_impl(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcounts, displs, recvtype, comm,
                                                     s);

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

    mpi_errno = MPIR_Ineighbor_alltoall_sched_impl(sendbuf, sendcount, sendtype, recvbuf,
                                                   recvcount, recvtype, comm, s);

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

    mpi_errno = MPIR_Ineighbor_alltoallv_sched_impl(sendbuf, sendcounts, sdispls,
                                                    sendtype, recvbuf, recvcounts, rdispls,
                                                    recvtype, comm, s);

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

    mpi_errno = MPIR_Ineighbor_alltoallw_sched_impl(sendbuf, sendcounts, sdispls,
                                                    sendtypes, recvbuf, recvcounts, rdispls,
                                                    recvtypes, comm, s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_NM_MPI_INEIGHBOR_ALLTOALLW_SCHED);
    return mpi_errno;
}

#endif /* OFI_COLL_H_INCLUDED */
