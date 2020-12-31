/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4_coll_impl.h"

MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BARRIER,
        .comm_ptr = comm,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BARRIER);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Barrier_impl(comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_alpha:
            mpi_errno = MPIDI_Barrier_intra_composition_alpha(comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Barrier_intra_composition_beta:
            mpi_errno = MPIDI_Barrier_intra_composition_beta(comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BARRIER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BCAST,
        .comm_ptr = comm,
        .u.bcast.buffer = buffer,
        .u.bcast.count = count,
        .u.bcast.datatype = datatype,
        .u.bcast.root = root,
    };

    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BCAST);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Bcast_impl(buffer, count, datatype, root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_alpha:
            mpi_errno =
                MPIDI_Bcast_intra_composition_alpha(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_beta:
            mpi_errno =
                MPIDI_Bcast_intra_composition_beta(buffer, count, datatype, root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Bcast_intra_composition_gamma:
            mpi_errno =
                MPIDI_Bcast_intra_composition_gamma(buffer, count, datatype, root, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BCAST);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLREDUCE,
        .comm_ptr = comm,

        .u.allreduce.sendbuf = sendbuf,
        .u.allreduce.recvbuf = recvbuf,
        .u.allreduce.count = count,
        .u.allreduce.datatype = datatype,
        .u.allreduce.op = op,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLREDUCE);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_alpha:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_beta:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                       comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allreduce_intra_composition_gamma:
            mpi_errno =
                MPIDI_Allreduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                        comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLREDUCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHER,
        .comm_ptr = comm,

        .u.allgather.sendbuf = sendbuf,
        .u.allgather.sendcount = sendcount,
        .u.allgather.sendtype = sendtype,
        .u.allgather.recvbuf = recvbuf,
        .u.allgather.recvcount = recvcount,
        .u.allgather.recvtype = recvtype,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHER);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm,
                                errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgather_intra_composition_alpha:
            mpi_errno =
                MPIDI_Allgather_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                        recvbuf, recvcount, recvtype,
                                                        comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const int *recvcounts, const int *displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLGATHERV,
        .comm_ptr = comm,

        .u.allgatherv.sendbuf = sendbuf,
        .u.allgatherv.sendcount = sendcount,
        .u.allgatherv.sendtype = sendtype,
        .u.allgatherv.recvbuf = recvbuf,
        .u.allgatherv.recvcounts = recvcounts,
        .u.allgatherv.displs = displs,
        .u.allgatherv.recvtype = recvtype,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHERV);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Allgatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
                                 recvtype, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Allgatherv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Allgatherv_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                         recvbuf, recvcounts, displs,
                                                         recvtype, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHERV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCATTER,
        .comm_ptr = comm,

        .u.scatter.sendbuf = sendbuf,
        .u.scatter.sendcount = sendcount,
        .u.scatter.sendtype = sendtype,
        .u.scatter.recvcount = recvcount,
        .u.scatter.recvbuf = recvbuf,
        .u.scatter.recvtype = recvtype,
        .u.scatter.root = root,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTER);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        MPIR_Scatter_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm,
                          errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatter_intra_composition_alpha:
            mpi_errno =
                MPIDI_Scatter_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcount, recvtype, root, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *sendbuf, const int *sendcounts,
                                           const int *displs, MPI_Datatype sendtype,
                                           void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCATTERV,
        .comm_ptr = comm,

        .u.scatterv.sendbuf = sendbuf,
        .u.scatterv.sendcounts = sendcounts,
        .u.scatterv.displs = displs,
        .u.scatterv.sendtype = sendtype,
        .u.scatterv.recvcount = recvcount,
        .u.scatterv.recvbuf = recvbuf,
        .u.scatterv.recvtype = recvtype,
        .u.scatterv.root = root,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTERV);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        MPIR_Scatterv_impl(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype,
                           root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scatterv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Scatterv_intra_composition_alpha(sendbuf, sendcounts, displs, sendtype,
                                                       recvbuf, recvcount, recvtype, root,
                                                       comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTERV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__GATHER,
        .comm_ptr = comm,

        .u.gather.sendbuf = sendbuf,
        .u.gather.sendcount = sendcount,
        .u.gather.sendtype = sendtype,
        .u.gather.recvcount = recvcount,
        .u.gather.recvbuf = recvbuf,
        .u.gather.recvtype = recvtype,
        .u.gather.root = root,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHER);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Gather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm,
                             errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gather_intra_composition_alpha:
            mpi_errno =
                MPIDI_Gather_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                     recvcount, recvtype, root, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const int *recvcounts, const int *displs,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__GATHERV,
        .comm_ptr = comm,

        .u.gatherv.sendbuf = sendbuf,
        .u.gatherv.sendcount = sendcount,
        .u.gatherv.sendtype = sendtype,
        .u.gatherv.recvbuf = recvbuf,
        .u.gatherv.recvcounts = recvcounts,
        .u.gatherv.displs = displs,
        .u.gatherv.recvtype = recvtype,
        .u.gatherv.root = root,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHERV);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Gatherv_impl(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                      displs, recvtype, root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Gatherv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Gatherv_intra_composition_alpha(sendbuf, sendcount, sendtype, recvbuf,
                                                      recvcounts, displs, recvtype, root,
                                                      comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHERV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALL,
        .comm_ptr = comm,

        .u.alltoall.sendbuf = sendbuf,
        .u.alltoall.sendcount = sendcount,
        .u.alltoall.sendtype = sendtype,
        .u.alltoall.recvcount = recvcount,
        .u.alltoall.recvbuf = recvbuf,
        .u.alltoall.recvtype = recvtype,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALL);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoall_intra_composition_alpha:
            mpi_errno =
                MPIDI_Alltoall_intra_composition_alpha(sendbuf, sendcount, sendtype,
                                                       recvbuf, recvcount, recvtype, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *sendbuf, const int *sendcounts,
                                            const int *sdispls, MPI_Datatype sendtype,
                                            void *recvbuf, const int *recvcounts,
                                            const int *rdispls, MPI_Datatype recvtype,
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALLV,
        .comm_ptr = comm,

        .u.alltoallv.sendbuf = sendbuf,
        .u.alltoallv.sendcounts = sendcounts,
        .u.alltoallv.sdispls = sdispls,
        .u.alltoallv.sendtype = sendtype,
        .u.alltoallv.recvbuf = recvbuf,
        .u.alltoallv.recvcounts = recvcounts,
        .u.alltoallv.rdispls = rdispls,
        .u.alltoallv.recvtype = recvtype,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLV);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Alltoallv_impl(sendbuf, sendcounts, sdispls,
                                        sendtype, recvbuf, recvcounts,
                                        rdispls, recvtype, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallv_intra_composition_alpha:
            mpi_errno =
                MPIDI_Alltoallv_intra_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *sendbuf, const int sendcounts[],
                                            const int sdispls[], const MPI_Datatype sendtypes[],
                                            void *recvbuf, const int recvcounts[],
                                            const int rdispls[], const MPI_Datatype recvtypes[],
                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLTOALLW,
        .comm_ptr = comm,

        .u.alltoallw.sendbuf = sendbuf,
        .u.alltoallw.sendcounts = sendcounts,
        .u.alltoallw.sdispls = sdispls,
        .u.alltoallw.sendtypes = sendtypes,
        .u.alltoallw.recvbuf = recvbuf,
        .u.alltoallw.recvcounts = recvcounts,
        .u.alltoallw.rdispls = rdispls,
        .u.alltoallw.recvtypes = recvtypes,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLW);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Alltoallw_impl(sendbuf, sendcounts, sdispls,
                                        sendtypes, recvbuf, recvcounts,
                                        rdispls, recvtypes, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Alltoallw_intra_composition_alpha:
            mpi_errno =
                MPIDI_Alltoallw_intra_composition_alpha(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLW);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *sendbuf, void *recvbuf,
                                         int count, MPI_Datatype datatype, MPI_Op op,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE,
        .comm_ptr = comm,

        .u.reduce.sendbuf = sendbuf,
        .u.reduce.recvbuf = recvbuf,
        .u.reduce.count = count,
        .u.reduce.datatype = datatype,
        .u.reduce.op = op,
        .u.reduce.root = root,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Reduce_impl(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_intra_composition_alpha(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_beta:
            mpi_errno =
                MPIDI_Reduce_intra_composition_beta(sendbuf, recvbuf, count, datatype, op,
                                                    root, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_intra_composition_gamma:
            mpi_errno =
                MPIDI_Reduce_intra_composition_gamma(sendbuf, recvbuf, count, datatype, op,
                                                     root, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                 const int recvcounts[], MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm,
                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER,
        .comm_ptr = comm,

        .u.reduce_scatter.sendbuf = sendbuf,
        .u.reduce_scatter.recvbuf = recvbuf,
        .u.reduce_scatter.recvcounts = recvcounts,
        .u.reduce_scatter.datatype = datatype,
        .u.reduce_scatter.op = op,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Reduce_scatter_impl(sendbuf, recvbuf, recvcounts, datatype, op, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_scatter_intra_composition_alpha(sendbuf, recvbuf, recvcounts,
                                                             datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                       int recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm,
                                                       MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK,
        .comm_ptr = comm,

        .u.reduce_scatter_block.sendbuf = sendbuf,
        .u.reduce_scatter_block.recvbuf = recvbuf,
        .u.reduce_scatter_block.recvcount = recvcount,
        .u.reduce_scatter_block.datatype = datatype,
        .u.reduce_scatter_block.op = op,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno =
            MPIR_Reduce_scatter_block_impl(sendbuf, recvbuf, recvcount, datatype, op, comm,
                                           errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Reduce_scatter_block_intra_composition_alpha:
            mpi_errno =
                MPIDI_Reduce_scatter_block_intra_composition_alpha(sendbuf, recvbuf, recvcount,
                                                                   datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                       MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__SCAN,
        .comm_ptr = comm,

        .u.scan.sendbuf = sendbuf,
        .u.scan.recvbuf = recvbuf,
        .u.scan.count = count,
        .u.scan.datatype = datatype,
        .u.scan.op = op,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCAN);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Scan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_alpha:
            mpi_errno =
                MPIDI_Scan_intra_composition_alpha(sendbuf, recvbuf, count,
                                                   datatype, op, comm, errflag);
            break;
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Scan_intra_composition_beta:
            mpi_errno =
                MPIDI_Scan_intra_composition_beta(sendbuf, recvbuf, count,
                                                  datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCAN);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    const MPIDI_Csel_container_s *cnt = NULL;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__EXSCAN,
        .comm_ptr = comm,

        .u.exscan.sendbuf = sendbuf,
        .u.exscan.recvbuf = recvbuf,
        .u.exscan.count = count,
        .u.exscan.datatype = datatype,
        .u.exscan.op = op,
    };

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_EXSCAN);

    cnt = MPIR_Csel_search(MPIDI_COMM(comm, csel_comm), coll_sig);

    if (cnt == NULL) {
        mpi_errno = MPIR_Exscan_impl(sendbuf, recvbuf, count, datatype, op, comm, errflag);;
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    switch (cnt->id) {
        case MPIDI_CSEL_CONTAINER_TYPE__COMPOSITION__MPIDI_Exscan_intra_composition_alpha:
            mpi_errno =
                MPIDI_Exscan_intra_composition_alpha(sendbuf, recvbuf, count,
                                                     datatype, op, comm, errflag);
            break;
        default:
            MPIR_Assert(0);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_EXSCAN);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgather(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_neighbor_allgather(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_allgatherv(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      const int *recvcounts, const int *displs,
                                                      MPI_Datatype recvtype, MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_neighbor_allgatherv(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                     const int *sdispls, MPI_Datatype sendtype,
                                                     void *recvbuf, const int *recvcounts,
                                                     const int *rdispls, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_neighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                     const MPI_Aint * sdispls,
                                                     const MPI_Datatype * sendtypes, void *recvbuf,
                                                     const int *recvcounts,
                                                     const MPI_Aint * rdispls,
                                                     const MPI_Datatype * recvtypes,
                                                     MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_neighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Neighbor_alltoall(const void *sendbuf, int sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    int recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_neighbor_alltoall(sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype, comm);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgather(const void *sendbuf, int sendcount,
                                                      MPI_Datatype sendtype, void *recvbuf,
                                                      int recvcount, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);

    ret = MPIDI_NM_mpi_ineighbor_allgather(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_allgatherv(const void *sendbuf, int sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       const int *recvcounts, const int *displs,
                                                       MPI_Datatype recvtype, MPIR_Comm * comm,
                                                       MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);

    ret = MPIDI_NM_mpi_ineighbor_allgatherv(sendbuf, sendcount, sendtype,
                                            recvbuf, recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoall(const void *sendbuf, int sendcount,
                                                     MPI_Datatype sendtype, void *recvbuf,
                                                     int recvcount, MPI_Datatype recvtype,
                                                     MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);

    ret = MPIDI_NM_mpi_ineighbor_alltoall(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallv(const void *sendbuf, const int *sendcounts,
                                                      const int *sdispls, MPI_Datatype sendtype,
                                                      void *recvbuf, const int *recvcounts,
                                                      const int *rdispls, MPI_Datatype recvtype,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);

    ret = MPIDI_NM_mpi_ineighbor_alltoallv(sendbuf, sendcounts, sdispls,
                                           sendtype, recvbuf, recvcounts, rdispls, recvtype, comm,
                                           req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ineighbor_alltoallw(const void *sendbuf, const int *sendcounts,
                                                      const MPI_Aint * sdispls,
                                                      const MPI_Datatype * sendtypes,
                                                      void *recvbuf, const int *recvcounts,
                                                      const MPI_Aint * rdispls,
                                                      const MPI_Datatype * recvtypes,
                                                      MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);

    ret = MPIDI_NM_mpi_ineighbor_alltoallw(sendbuf, sendcounts, sdispls,
                                           sendtypes, recvbuf, recvcounts, rdispls, recvtypes,
                                           comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_INEIGHBOR_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBARRIER);

    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBCAST);

    ret = MPIDI_NM_mpi_ibcast(buffer, count, datatype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBCAST);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLGATHER);

    ret = MPIDI_NM_mpi_iallgather(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLGATHERV);

    ret = MPIDI_NM_mpi_iallgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                   recvcounts, displs, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLREDUCE);

    ret = MPIDI_NM_mpi_iallreduce(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALL);

    ret = MPIDI_NM_mpi_ialltoall(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALLV);

    ret = MPIDI_NM_mpi_ialltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                  recvbuf, recvcounts, rdispls, recvtype, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ialltoallw(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, const MPI_Datatype * sendtypes,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, const MPI_Datatype * recvtypes,
                                             MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IALLTOALLW);

    ret = MPIDI_NM_mpi_ialltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                  recvbuf, recvcounts, rdispls, recvtypes, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iexscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IEXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IEXSCAN);

    ret = MPIDI_NM_mpi_iexscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IEXSCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igather(const void *sendbuf, int sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                          MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                          MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IGATHER);

    ret = MPIDI_NM_mpi_igather(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Igatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IGATHERV);

    ret = MPIDI_NM_mpi_igatherv(sendbuf, sendcount, sendtype, recvbuf,
                                recvcounts, displs, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm,
                                                        MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int *recvcounts, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE_SCATTER);

    ret = MPIDI_NM_mpi_ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ireduce(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IREDUCE);

    ret = MPIDI_NM_mpi_ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IREDUCE);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCAN);

    ret = MPIDI_NM_mpi_iscan(sendbuf, recvbuf, count, datatype, op, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTER);

    ret = MPIDI_NM_mpi_iscatter(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Iscatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm, MPIR_Request ** req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTERV);

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bcast_init(void *buffer, int count, MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm_ptr, MPIR_Info * info_ptr,
                                             MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BCAST_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BCAST_INIT);

    mpi_errno = MPIR_Bcast_init(buffer, count, datatype, root, comm_ptr, info_ptr, request);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BCAST_INIT);
    return mpi_errno;
}

#endif /* CH4_COLL_H_INCLUDED */
