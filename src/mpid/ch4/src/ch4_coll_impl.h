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
#ifndef CH4_COLL_IMPL_H_INCLUDED
#define CH4_COLL_IMPL_H_INCLUDED

#include "ch4_coll_params.h"
#include "coll_algo_params.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_Barrier_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_composition_alpha(MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                             MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * barrier_node_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    void * barrier_roots_container = MPIDI_coll_get_next_container(barrier_node_container);
    void * bcast_node_container = MPIDI_coll_get_next_container(barrier_roots_container);

    /* do the intranode barrier on all nodes */
    if (comm->node_comm != NULL)
    {
#ifdef MPIDI_BUILD_CH4_SHM
      mpi_errno =
          MPIDI_SHM_mpi_barrier(comm->node_comm, errflag, barrier_node_container);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
      mpi_errno =
          MPIDI_NM_mpi_barrier(comm->node_comm, errflag, barrier_node_container);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }

    /* do the barrier across roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_barrier(comm->node_roots_comm, errflag, barrier_roots_container);
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
            MPIDI_SHM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag,
                                bcast_node_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag,
                               bcast_node_container);
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
                                                            MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * barrier_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
        MPIDI_NM_mpi_barrier(comm, errflag, barrier_container);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node) {
        mpi_errno =
            MPIDI_SHM_mpi_barrier(comm, errflag, barrier_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else{
        mpi_errno =
            MPIDI_NM_mpi_barrier(comm, errflag, barrier_container);
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
                                                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * bcast_roots_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    void * bcast_node_container = MPIDI_coll_get_next_container(bcast_roots_container);

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
                               comm->node_roots_comm, errflag, bcast_roots_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    if (comm->node_comm != NULL) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                                bcast_node_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                               bcast_node_container);
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
                                                          MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * bcast_roots_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    void * bcast_node_container_root_local = MPIDI_coll_get_next_container(bcast_roots_container);
    void * bcast_node_container_root_remote = MPIDI_coll_get_next_container(bcast_node_container_root_local);

    if (comm->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm, root) > 0) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag, bcast_node_container_root_local);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                               comm->node_comm, errflag, bcast_node_container_root_local);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
    }
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag, bcast_roots_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    if (comm->node_comm != NULL &&
        MPIR_Get_intranode_rank(comm, root) <= 0){
#ifdef MPIDI_BUILD_CH4_SHM
      mpi_errno =
          MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                              bcast_node_container_root_remote);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
      mpi_errno =
          MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                             bcast_node_container_root_remote);
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
                                                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * bcast_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
        MPIDI_NM_mpi_bcast(buffer, count, datatype, root,
                           comm, errflag, bcast_container);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node){
      mpi_errno =
          MPIDI_SHM_mpi_bcast(buffer, count, datatype, root,
                              comm, errflag, bcast_container);
      if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else{
      mpi_errno =
          MPIDI_NM_mpi_bcast(buffer, count, datatype, root,
                             comm, errflag, bcast_container);
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

#undef FUNCNAME
#define FUNCNAME MPIDI_Allreduce_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_composition_alpha(const void *sendbuf, void *recvbuf, int count,
                                                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * reduce_node_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    void * allred_roots_container = MPIDI_coll_get_next_container(reduce_node_container);
    void * bcast_node_container = MPIDI_coll_get_next_container(allred_roots_container);

    if(comm->node_comm != NULL) {
        if((sendbuf == MPI_IN_PLACE) && (comm->node_comm->rank != 0)) {
#ifdef MPIDI_BUILD_CH4_SHM
          mpi_errno = MPIDI_SHM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm, errflag,
                                           reduce_node_container);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
          mpi_errno = MPIDI_NM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm, errflag,
                                          reduce_node_container);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#endif/* MPIDI_BUILD_CH4_SHM */
        } else {
#ifdef MPIDI_BUILD_CH4_SHM
          mpi_errno = MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm, errflag,
                                           reduce_node_container);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
          mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm, errflag,
                                          reduce_node_container);
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
        mpi_errno = MPIDI_NM_mpi_allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op, comm->node_roots_comm,
                                           errflag, allred_roots_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    if (comm->node_comm != NULL) {
#ifdef MPIDI_BUILD_CH4_SHM
        mpi_errno = MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag,
                                        bcast_node_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno = MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag,
                                       bcast_node_container);
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
                                                              MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
   int mpi_errno = MPI_SUCCESS;
   void * allred_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
         MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                allred_container);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node) {
        mpi_errno =
             MPIDI_SHM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                     allred_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno =
             MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                    allred_container);
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

#undef FUNCNAME
#define FUNCNAME MPIDI_Reduce_composition_alpha
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_composition_alpha(const void *sendbuf, void *recvbuf, int count,
                                                            MPI_Datatype datatype, MPI_Op op, int root,
                                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                            MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_lb, true_extent, extent;
    void * reduce_roots_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    void * reduce_node_container = MPIDI_coll_get_next_container(reduce_roots_container);

    MPIR_CHKLMEM_DECL(1);

    void *tmp_buf = NULL;

    /* Create a temporary buffer on local roots of all nodes */
    if (comm->node_roots_comm != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

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
            MPIDI_SHM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag,
                                 reduce_node_container);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag,
                                reduce_node_container);
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
                                    comm->node_roots_comm, errflag, reduce_roots_container);
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
                                        comm->node_roots_comm, errflag, reduce_roots_container);

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
                                        comm->node_roots_comm, errflag, reduce_roots_container);
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
                                 comm->node_comm, errflag, reduce_node_container);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                op, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag, reduce_node_container);
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
                                                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * reduce_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

#ifndef MPIDI_BUILD_CH4_SHM
    mpi_errno =
        MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                            comm, errflag, reduce_container);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
#else
    if(comm->is_single_node) {
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                                 comm, errflag, reduce_container);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                                comm, errflag, reduce_container);
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

#endif /* CH4_COLL_IMPL_H_INCLUDED */
