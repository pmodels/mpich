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
#ifndef CH4_COLL_H_INCLUDED
#define CH4_COLL_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"

MPL_STATIC_INLINE_PREFIX int MPID_Barrier(MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BARRIER);

    ch4_algo_parameters_container = MPIDI_CH4_Barrier_select(comm, errflag);

    mpi_errno =
        MPIDI_CH4_Barrier_call(comm, errflag, ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BARRIER);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPID_Bcast(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_BCAST);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_BCAST);

    ch4_algo_parameters_container = MPIDI_CH4_Bcast_select(buffer, count, datatype, root, comm, errflag);

    mpi_errno =
        MPIDI_CH4_Bcast_call(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_BCAST);

    return mpi_errno;

}

#undef FUNCNAME
#define FUNCNAME MPIDI_Barrier_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_composition_alpha(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                             MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    /* do the intranode barrier on all nodes */
    if (comm->node_comm != NULL)
    {
#ifdef MPIDI_BUILD_CH4_SHM
      mpi_errno =
          MPIDI_SHM_mpi_barrier(comm->node_comm, errflag);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
      mpi_errno =
          MPIDI_NM_mpi_barrier(comm->node_comm, errflag);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }

    /* do the barrier across roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_barrier(comm->node_roots_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* release the local processes on each node with a 1-byte
       broadcast (0-byte broadcast just returns without doing
       anything) */
    if (comm->node_comm != NULL)
    {
        int i=0;
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Barrier_composition_beta
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_composition_beta(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                            MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
        MPIDI_NM_mpi_barrier(comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node) {
        mpi_errno =
            MPIDI_SHM_mpi_barrier(comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else{
        mpi_errno =
            MPIDI_NM_mpi_barrier(comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
#endif/*MPIDI_BUILD_CH4_SHM*/

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Barrier_intercomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intercomm(MPIR_Comm * comm, MPIR_Errflag_t *errflag,
                                                     MPIDI_coll_algo_container_t *ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier_inter(comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Bcast_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_alpha(void *buffer, int count, MPI_Datatype datatype,
                                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    int type_size, nbytes;

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE ? type_size*count : 0;

    if (comm->node_roots_comm == NULL && comm->rank == root) {
        mpi_errno = MPIC_Send(buffer, count, datatype, 0, MPIR_BCAST_TAG, comm->node_comm, errflag);
    }

    if (comm->node_roots_comm != NULL && comm->rank != root && MPIR_Get_intranode_rank(comm, root) != -1){
        mpi_errno = MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root), MPIR_BCAST_TAG,
                              comm->node_comm, MPI_STATUS_IGNORE, errflag);
    }

    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    if (comm->node_comm != NULL) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Bcast_composition_beta
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_beta(void *buffer, int count, MPI_Datatype datatype,
                                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                          MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    if (comm->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm, root) > 0) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                               comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    if (comm->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm, root) <= 0){
#ifdef MPIDI_BUILD_CH4_SHM
      mpi_errno =
          MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
      mpi_errno =
          MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Bcast_composition_gamma
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_gamma(void *buffer, int count, MPI_Datatype datatype,
                                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
        MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node){
      mpi_errno =
          MPIDI_SHM_mpi_bcast(buffer, count, datatype, root, comm, errflag);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else{
      mpi_errno =
          MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
#endif/*MPIDI_BUILD_CH4_SHM*/

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Bcast_intercomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intercomm(void *buffer, int count, MPI_Datatype datatype,
                                                   int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_inter(buffer, count, datatype, root, comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret = MPI_SUCCESS;
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLREDUCE);

    ch4_algo_parameters_container =
        MPIDI_CH4_Allreduce_select(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    ret = MPIDI_CH4_Allreduce_call(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                   ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLREDUCE);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Allreduce_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_composition_alpha(const void *sendbuf, void *recvbuf, int count,
                                                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    if(comm->node_comm != NULL) {
        if((sendbuf == MPI_IN_PLACE) && (comm->node_comm->rank != 0)) {
#ifdef MPIDI_BUILD_CH4_SHM
          mpi_errno = MPIDI_SHM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm, errflag);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
          mpi_errno = MPIDI_NM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm, errflag);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
        } else {
#ifdef MPIDI_BUILD_CH4_SHM
          mpi_errno = MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm, errflag);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
          mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm, errflag);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
        }
    } else {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    if (comm->node_roots_comm != NULL) {
        mpi_errno = MPIDI_NM_mpi_allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op, comm->node_roots_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    if (comm->node_comm != NULL) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno = MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno = MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Allreduce_composition_gamma
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_composition_beta(const void *sendbuf, void *recvbuf, int count,
                                                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
   int mpi_errno = MPI_SUCCESS;

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
         MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node) {
        mpi_errno =
             MPIDI_SHM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno =
             MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
#endif/*MPIDI_BUILD_CH4_SHM*/

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Allreduce_intercomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intercomm(const void *sendbuf, void *recvbuf, int count,
                                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                       MPIR_Errflag_t * errflag,
                                                       MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_inter(sendbuf, recvbuf, count, datatype, op, comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgather(const void *sendbuf, int sendcount,
                                             MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                             MPI_Datatype recvtype, MPIR_Comm * comm,
                                             MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHER);

    ret = MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype, recvbuf,
                                 recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Allgatherv(const void *sendbuf, int sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              const int *recvcounts, const int *displs,
                                              MPI_Datatype recvtype, MPIR_Comm * comm,
                                              MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLGATHERV);

    ret = MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype, recvbuf,
                                  recvcounts, displs, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLGATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatter(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTER);

    ret = MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf,
                               recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scatterv(const void *sendbuf, const int *sendcounts,
                                            const int *displs, MPI_Datatype sendtype,
                                            void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCATTERV);

    ret = MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype,
                                recvbuf, recvcount, recvtype, root, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCATTERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                                          void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHER);

    ret = MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Gatherv(const void *sendbuf, int sendcount,
                                           MPI_Datatype sendtype, void *recvbuf,
                                           const int *recvcounts, const int *displs,
                                           MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                           MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_GATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_GATHERV);

    ret = MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf,
                               recvcounts, displs, recvtype, root, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_GATHERV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoall(const void *sendbuf, int sendcount,
                                            MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                            MPI_Datatype recvtype, MPIR_Comm * comm,
                                            MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALL);

    ret = MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                                recvcount, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALL);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallv(const void *sendbuf, const int *sendcounts,
                                             const int *sdispls, MPI_Datatype sendtype,
                                             void *recvbuf, const int *recvcounts,
                                             const int *rdispls, MPI_Datatype recvtype,
                                             MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLV);

    ret = MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls, sendtype,
                                 recvbuf, recvcounts, rdispls, recvtype, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLV);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Alltoallw(const void *sendbuf, const int sendcounts[],
                                             const int sdispls[], const MPI_Datatype sendtypes[],
                                             void *recvbuf, const int recvcounts[],
                                             const int rdispls[], const MPI_Datatype recvtypes[],
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ALLTOALLW);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ALLTOALLW);

    ret = MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes,
                                 recvbuf, recvcounts, rdispls, recvtypes, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ALLTOALLW);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce(const void *sendbuf, void *recvbuf,
                                         int count, MPI_Datatype datatype, MPI_Op op,
                                         int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int ret = MPI_SUCCESS;
    MPIDI_coll_algo_container_t *ch4_algo_parameters_container = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE);

    ch4_algo_parameters_container =
        MPIDI_CH4_Reduce_select(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);

    ret = MPIDI_CH4_Reduce_call(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag,
                                ch4_algo_parameters_container);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE);
    return ret;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Reduce_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_composition_alpha(const void *sendbuf, void *recvbuf, int count,
                                                            MPI_Datatype datatype, MPI_Op op, int root,
                                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                            MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_lb, true_extent, extent;

    MPIR_CHKLMEM_DECL(1);

    void *tmp_buf = NULL;

    /* Create a temporary buffer on local roots of all nodes */
    if (comm->node_roots_comm != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPID_Datatype_get_extent_macro(datatype, extent);

        MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)),
                            mpi_errno, "temporary buffer");
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *)((char*)tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (comm->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm, root) == -1) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag);
#endif/* MPIDI_BUILD_CH4_SHM */
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* do the internode reduce to the root's node */
    if (comm->node_roots_comm != NULL) {
        if (comm->node_roots_comm->rank != MPIR_Get_internode_rank(comm, root)) {
            /* I am not on root's node.  Use tmp_buf if we
               participated in the first reduce, otherwise use sendbuf */
            const void *buf = (comm->node_comm == NULL ? sendbuf : tmp_buf);
            mpi_errno =
                MPIDI_NM_mpi_reduce(buf, NULL, count, datatype,
                                    op, MPIR_Get_internode_rank(comm, root),
                                    comm->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        else { /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                   Use tmp_buf as recvbuf. */
                mpi_errno =
                    MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag);

                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            }
            else {
                /* I am the root. in_place is automatically handled. */
                mpi_errno =
                    MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }

    }

    /* do the intranode reduce on the root's node */
    if (comm->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm, root) != -1) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                 op, MPIR_Get_intranode_rank(comm, root),
                                 comm->node_comm, errflag);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                op, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag);
#endif/* MPIDI_BUILD_CH4_SHM */
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;

}


#undef FUNCNAME
#define FUNCNAME MPIDI_Reduce_composition_beta
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_composition_beta(const void *sendbuf, void *recvbuf, int count,
                                                           MPI_Datatype datatype, MPI_Op op, int root,
                                                           MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
        MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                            comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node) {
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                                 comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                                comm, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
#endif/*MPIDI_BUILD_CH4_SHM*/

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_Reduce_intercomm
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intercomm(const void *sendbuf, void *recvbuf, int count,
                                                    MPI_Datatype datatype, MPI_Op op, int root,
                                                    MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                    MPIDI_coll_algo_container_t * ch4_algo_parameters_container ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_inter(sendbuf, recvbuf, count, datatype,
                                  op, root, comm, errflag);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter(const void *sendbuf, void *recvbuf,
                                                  const int recvcounts[], MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER);

    ret = MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr,
                                      errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Reduce_scatter_block(const void *sendbuf, void *recvbuf,
                                                        int recvcount, MPI_Datatype datatype,
                                                        MPI_Op op, MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);

    ret = MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount,
                                            datatype, op, comm_ptr, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_REDUCE_SCATTER_BLOCK);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Scan(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                        MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_SCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_SCAN);

    ret = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_SCAN);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Exscan(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_EXSCAN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_EXSCAN);

    ret = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm, errflag);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_EXSCAN);
    return ret;
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
                                                       MPIR_Comm * comm, MPI_Request * req)
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
                                                        MPI_Request * req)
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
                                                      MPIR_Comm * comm, MPI_Request * req)
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
                                                       MPIR_Comm * comm, MPI_Request * req)
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
                                                       MPIR_Comm * comm, MPI_Request * req)
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

MPL_STATIC_INLINE_PREFIX int MPID_Ibarrier(MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_IBARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_IBARRIER);

    ret = MPIDI_NM_mpi_ibarrier(comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_IBARRIER);
    return ret;
}

MPL_STATIC_INLINE_PREFIX int MPID_Ibcast(void *buffer, int count, MPI_Datatype datatype,
                                          int root, MPIR_Comm * comm, MPI_Request * req)
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
                                              MPI_Request * req)
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
                                               MPI_Request * req)
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
                                              MPI_Request * req)
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
                                             MPI_Request * req)
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
                                              MPIR_Comm * comm, MPI_Request * req)
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
                                              MPIR_Comm * comm, MPI_Request * req)
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
                                           MPI_Request * req)
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
                                           MPI_Request * req)
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
                                            MPI_Request * req)
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
                                                         MPI_Request * req)
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
                                                   MPI_Op op, MPIR_Comm * comm, MPI_Request * req)
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
                                           MPIR_Comm * comm, MPI_Request * req)
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
                                         MPI_Request * req)
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
                                            MPI_Request * req)
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
                                             int root, MPIR_Comm * comm, MPI_Request * req)
{
    int ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_ISCATTERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_ISCATTERV);

    ret = MPIDI_NM_mpi_iscatterv(sendbuf, sendcounts, displs, sendtype,
                                 recvbuf, recvcount, recvtype, root, comm, req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_ISCATTERV);
    return ret;
}

#endif /* CH4_COLL_H_INCLUDED */
