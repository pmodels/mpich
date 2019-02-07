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

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLTOALL_SHM_PER_RANK
      category    : COLLECTIVE
      type        : int
      default     : 4096
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Shared memory region per rank for multi-leaders based composition for MPI_Alltoall (in bytes)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#ifndef CH4_COLL_IMPL_H_INCLUDED
#define CH4_COLL_IMPL_H_INCLUDED

#include "ch4_coll_params.h"
#include "coll_algo_params.h"
#include "ch4_comm.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intra_composition_alpha(MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *barrier_node_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *barrier_roots_container = MPIDI_coll_get_next_container(barrier_node_container);
    const void *bcast_node_container = MPIDI_coll_get_next_container(barrier_roots_container);

    /* do the intranode barrier on all nodes */
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_mpi_barrier(comm->node_comm, errflag, barrier_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno = MPIDI_NM_mpi_barrier(comm->node_comm, errflag, barrier_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    /* do the barrier across roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        mpi_errno = MPIDI_NM_mpi_barrier(comm->node_roots_comm, errflag, barrier_roots_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (comm->node_comm != NULL) {
        int i = 0;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag, bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag, bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intra_composition_beta(MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *barrier_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno = MPIDI_NM_mpi_barrier(comm, errflag, barrier_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_inter_composition_alpha(MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   *
                                                                   ch4_algo_parameters_container
                                                                   ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Barrier_inter_auto(comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_alpha(void *buffer, int count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag,
                                                                 const
                                                                 MPIDI_coll_algo_container_t
                                                                 * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *bcast_roots_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *bcast_node_container = MPIDI_coll_get_next_container(bcast_roots_container);

    if (comm->node_roots_comm == NULL && comm->rank == root) {
        mpi_errno = MPIC_Send(buffer, count, datatype, 0, MPIR_BCAST_TAG, comm->node_comm, errflag);
    }

    if (comm->node_roots_comm != NULL && comm->rank != root &&
        MPIR_Get_intranode_rank(comm, root) != -1) {
        mpi_errno =
            MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root), MPIR_BCAST_TAG,
                      comm->node_comm, MPI_STATUS_IGNORE, errflag);
    }

    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag, bcast_roots_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                                bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                               bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_beta(void *buffer, int count,
                                                                MPI_Datatype datatype,
                                                                int root, MPIR_Comm * comm,
                                                                MPIR_Errflag_t * errflag,
                                                                const
                                                                MPIDI_coll_algo_container_t
                                                                * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *bcast_roots_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *bcast_node_container_root_local =
        MPIDI_coll_get_next_container(bcast_roots_container);
    const void *bcast_node_container_root_remote =
        MPIDI_coll_get_next_container(bcast_node_container_root_local);

    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) > 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag, bcast_node_container_root_local);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                               comm->node_comm, errflag, bcast_node_container_root_local);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag, bcast_roots_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) <= 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                                bcast_node_container_root_remote);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag,
                               bcast_node_container_root_remote);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_gamma(void *buffer, int count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag,
                                                                 const
                                                                 MPIDI_coll_algo_container_t
                                                                 * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *bcast_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag, bcast_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_inter_composition_alpha(void *buffer, int count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag,
                                                                 const
                                                                 MPIDI_coll_algo_container_t
                                                                 *
                                                                 ch4_algo_parameters_container
                                                                 ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Bcast_inter_auto(buffer, count, datatype, root, comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_alpha(const void *sendbuf,
                                                                     void *recvbuf, int count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *reduce_node_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *allred_roots_container = MPIDI_coll_get_next_container(reduce_node_container);
    const void *bcast_node_container = MPIDI_coll_get_next_container(allred_roots_container);

    if (comm->node_comm != NULL) {
        if ((sendbuf == MPI_IN_PLACE) && (comm->node_comm->rank != 0)) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno =
                MPIDI_SHM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm,
                                     errflag, reduce_node_container);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
#else
            mpi_errno =
                MPIDI_NM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm, errflag,
                                    reduce_node_container);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        } else {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno =
                MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                     errflag, reduce_node_container);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
#else
            mpi_errno =
                MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                    errflag, reduce_node_container);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        }
    } else {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op,
                                   comm->node_roots_comm, errflag, allred_roots_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag,
                                        bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno = MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag,
                                       bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_beta(const void *sendbuf,
                                                                    void *recvbuf, int count,
                                                                    MPI_Datatype datatype,
                                                                    MPI_Op op,
                                                                    MPIR_Comm * comm,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_coll_algo_container_t
                                                                    * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *allred_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                               allred_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_gamma(const void *sendbuf,
                                                                     void *recvbuf, int count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *allred_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno =
        MPIDI_SHM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                allred_container);
#else
    mpi_errno =
        MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                               allred_container);
#endif
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_inter_composition_alpha(const void *sendbuf,
                                                                     void *recvbuf, int count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container
                                                                     ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allreduce_inter_auto(sendbuf, recvbuf, count, datatype, op, comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf, int count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    const void *reduce_roots_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *reduce_node_container = MPIDI_coll_get_next_container(reduce_roots_container);
    const void *inter_sendbuf;
    void *ori_recvbuf = recvbuf;

    MPIR_CHKLMEM_DECL(1);

    /* Create a temporary buffer on local roots of all nodes,
     * except for root if it is also a local root */
    if (comm->node_roots_comm != NULL && comm->rank != root) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(recvbuf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    /* intranode reduce on all nodes */
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                         errflag, reduce_node_container);
#else
        mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                        errflag, reduce_node_container);
#endif /* MPIDI_CH4_DIRECT_NETMOD */

        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        /* recvbuf becomes the sendbuf for internode reduce */
        inter_sendbuf = recvbuf;
    } else {
        inter_sendbuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
    }

    /* internode reduce with rank 0 in node_roots_comm as the root */
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_reduce(comm->node_roots_comm->rank == 0 ? MPI_IN_PLACE : inter_sendbuf,
                                recvbuf, count, datatype, op, 0, comm->node_roots_comm, errflag,
                                reduce_roots_container);

        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Send data to root via point-to-point message if root is not rank 0 in comm */
    if (root != 0) {
        if (comm->rank == 0) {
            MPIC_Send(recvbuf, count, datatype, root, MPIR_REDUCE_TAG, comm, errflag);
        } else if (comm->rank == root) {
            MPIC_Recv(ori_recvbuf, count, datatype, 0, MPIR_REDUCE_TAG, comm, MPI_STATUS_IGNORE,
                      errflag);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_beta(const void *sendbuf,
                                                                 void *recvbuf, int count,
                                                                 MPI_Datatype datatype,
                                                                 MPI_Op op, int root,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag,
                                                                 const
                                                                 MPIDI_coll_algo_container_t
                                                                 * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    const void *reduce_roots_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *reduce_node_container = MPIDI_coll_get_next_container(reduce_roots_container);

    MPIR_CHKLMEM_DECL(1);

    void *tmp_buf = NULL;

    /* Create a temporary buffer on local roots of all nodes */
    if (comm->node_roots_comm != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) == -1) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag,
                                 reduce_node_container);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag,
                                reduce_node_container);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* do the internode reduce to the root's node */
    if (comm->node_roots_comm != NULL) {
        if (comm->node_roots_comm->rank != MPIR_Get_internode_rank(comm, root)) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (comm->node_comm == NULL ? sendbuf : tmp_buf);
            mpi_errno =
                MPIDI_NM_mpi_reduce(buf, NULL, count, datatype,
                                    op, MPIR_Get_internode_rank(comm, root),
                                    comm->node_roots_comm, errflag, reduce_roots_container);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        } else {        /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                 * Use tmp_buf as recvbuf. */
                mpi_errno =
                    MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag, reduce_roots_container);

                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            } else {
                /* I am the root. in_place is automatically handled. */
                mpi_errno =
                    MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag, reduce_roots_container);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }

    }

    /* do the intranode reduce on the root's node */
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) != -1) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                 op, MPIR_Get_intranode_rank(comm, root),
                                 comm->node_comm, errflag, reduce_node_container);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                op, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag, reduce_node_container);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
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


MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_gamma(const void *sendbuf,
                                                                  void *recvbuf, int count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *reduce_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root,
                            comm, errflag, reduce_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_inter_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf, int count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  *
                                                                  ch4_algo_parameters_container
                                                                  ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_inter_auto(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Node-aware multi-leaders based inter-node and intra-node composition. Each rank on a node places
 * the data for ranks sitting on other nodes into a shared memory buffer. Next each rank participates
 * as a leader in inter-node Alltoall */
MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_intra_composition_alpha(const void *sendbuf,
                                                                    int sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    int recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_coll_algo_container_t
                                                                    * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    const void *barrier_node_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *alltoall_container = MPIDI_coll_get_next_container(barrier_node_container);
    int num_nodes;
    int num_ranks = MPIR_Comm_size(comm_ptr);
    int node_comm_size = MPIR_Comm_size(comm_ptr->node_comm);
    int my_node_comm_rank = MPIR_Comm_rank(comm_ptr->node_comm);
    int i, j, p = 0;
    MPI_Aint type_size;
    bool mapfail_flag = false;

    if (sendcount == 0)
        goto fn_exit;

    if (sendbuf != MPI_IN_PLACE) {
        MPIR_Datatype_get_size_macro(sendtype, type_size);
    } else {
        MPIR_Datatype_get_size_macro(recvtype, type_size);
    }

    num_nodes = MPIDI_COMM(comm_ptr, spanned_num_nodes);
    if (sendbuf == MPI_IN_PLACE) {
        sendbuf = recvbuf;
        sendcount = recvcount;
        sendtype = recvtype;
    }

    if (MPIDI_COMM(comm_ptr, multi_leads_comm) == NULL) {
        /* Create multi-leaders comm in a lazy manner */
        mpi_errno = MPIDI_Comm_create_multi_leaders(comm_ptr);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Allocate the shared memory buffer per node, if it is not already done */
    if (MPIDI_COMM(comm_ptr, alltoall_comp_info->shm_addr) == NULL) {
        MPIDI_COMM(comm_ptr, alltoall_comp_info->shm_size) =
            node_comm_size * num_ranks * MPIR_CVAR_ALLTOALL_SHM_PER_RANK;
        mpi_errno =
            MPIDIU_allocate_shm_segment(comm_ptr->node_comm,
                                        MPIDI_COMM_ALLTOALL(comm_ptr, shm_size),
                                        &MPIDI_COMM_ALLTOALL(comm_ptr, shm_handle), (void **)
                                        &MPIDI_COMM_ALLTOALL(comm_ptr, shm_addr), &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Barrier to make sure that the shm buffer can be reused after the previous call to Alltoall */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag, barrier_node_container);
#else
    mpi_errno = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag, barrier_node_container);
#endif
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* Each rank on a node copy its data into shm buffer */
    /* Example - 2 ranks per node on 2 nodes. R0 and R1 on node 0, R2 and R3 on node 1.
     * R0 buf is (0, 4, 8, 12). R1 buf is (1, 5, 9, 13). R2 buf is (2, 6, 10, 14) and R3 buf is
     * (3, 7, 11, 15). In shm_buf of node 0, place data from (R0, R1) for R0, R2, R1, and R3. In
     * shm_buf of node 1, place data from (R2, R3) for R0, R2, R1, R3. The node 0 shm_buf becomes
     * (0, 1, 8, 9, 4, 5, 12, 13). The node 1 shm_buf becomes (2, 3, 10, 11, 6, 7, 14, 15). */
    for (i = 0; i < node_comm_size; i++) {
        for (j = 0; j < num_nodes; j++) {
            mpi_errno = MPIR_Localcopy((void *) ((char *) sendbuf +
                                                 (i + j * node_comm_size) * type_size * sendcount),
                                       sendcount, sendtype, (void *) ((char *)
                                                                      MPIDI_COMM_ALLTOALL(comm_ptr,
                                                                                          shm_addr)
                                                                      + (p * node_comm_size +
                                                                         my_node_comm_rank) *
                                                                      type_size * sendcount),
                                       sendcount, sendtype);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            p++;
        }
    }

    /* Barrier to make sure each rank has copied the data to the shm buf */
#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_barrier(comm_ptr->node_comm, errflag, barrier_node_container);
#else
    mpi_errno = MPIDI_NM_mpi_barrier(comm_ptr->node_comm, errflag, barrier_node_container);
#endif
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* Call internode alltoall on the shm_bufs and multi-leaders communicator */
    /* In the above example, first half on shm_bufs are used by the first multi-leader comm of R0
     * and R2 for Alltoall. Second half is used by R1 and R3, which is in the second multi-leader
     * comm. That is, for the alltoall, R1's buf is (4, 5, 12, 13) and R3's buf is (6, 7, 14, 15).
     * After Alltoall R1's buf is (4, 5, 6, 7) and R3's buf is (12, 13, 14, 15), which is the
     * expected result */
    mpi_errno = MPIDI_NM_mpi_alltoall((void *) ((char *)
                                                MPIDI_COMM_ALLTOALL(comm_ptr,
                                                                    shm_addr) +
                                                my_node_comm_rank * num_nodes * node_comm_size *
                                                type_size * sendcount), node_comm_size * sendcount,
                                      sendtype, recvbuf, sendcount * node_comm_size, sendtype,
                                      MPIDI_COMM(comm_ptr, multi_leads_comm), errflag,
                                      alltoall_container);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

  fn_exit:
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_intra_composition_beta(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   int recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *alltoall_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, comm_ptr, errflag, alltoall_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_inter_composition_alpha(const void *sendbuf,
                                                                    int sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    int recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_coll_algo_container_t
                                                                    *
                                                                    ch4_algo_parameters_container
                                                                    ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoall_inter_auto(sendbuf, sendcount, sendtype, recvbuf,
                                         recvcount, recvtype, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallv_intra_composition_alpha(const void *sendbuf,
                                                                     const int *sendcounts,
                                                                     const int *sdispls,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const int *recvcounts,
                                                                     const int *rdispls,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *alltoallv_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls,
                               sendtype, recvbuf, recvcounts,
                               rdispls, recvtype, comm_ptr, errflag, alltoallv_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallv_inter_composition_alpha(const void *sendbuf,
                                                                     const int *sendcounts,
                                                                     const int *sdispls,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const int *recvcounts,
                                                                     const int *rdispls,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container
                                                                     ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallv_inter_auto(sendbuf, sendcounts, sdispls,
                                          sendtype, recvbuf, recvcounts,
                                          rdispls, recvtype, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallw_intra_composition_alpha(const void *sendbuf,
                                                                     const int sendcounts[],
                                                                     const int sdispls[],
                                                                     const MPI_Datatype
                                                                     sendtypes[],
                                                                     void *recvbuf,
                                                                     const int recvcounts[],
                                                                     const int rdispls[],
                                                                     const MPI_Datatype
                                                                     recvtypes[],
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *alltoallw_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls,
                               sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes, comm_ptr, errflag, alltoallw_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallw_inter_composition_alpha(const void *sendbuf,
                                                                     const int sendcounts[],
                                                                     const int sdispls[],
                                                                     const MPI_Datatype
                                                                     sendtypes[],
                                                                     void *recvbuf,
                                                                     const int recvcounts[],
                                                                     const int rdispls[],
                                                                     const MPI_Datatype
                                                                     recvtypes[],
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container
                                                                     ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Alltoallw_inter_auto(sendbuf, sendcounts, sdispls,
                                          sendtypes, recvbuf, recvcounts,
                                          rdispls, recvtypes, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather_intra_composition_alpha(const void *sendbuf,
                                                                     int sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     int recvcount,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *allgather_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype,
                               recvbuf, recvcount, recvtype,
                               comm_ptr, errflag, allgather_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather_inter_composition_alpha(const void *sendbuf,
                                                                     int sendcount,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     int recvcount,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag,
                                                                     const
                                                                     MPIDI_coll_algo_container_t
                                                                     *
                                                                     ch4_algo_parameters_container
                                                                     ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_inter_auto(sendbuf, sendcount, sendtype,
                                          recvbuf, recvcount, recvtype, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgatherv_intra_composition_alpha(const void *sendbuf,
                                                                      int sendcount,
                                                                      MPI_Datatype sendtype,
                                                                      void *recvbuf,
                                                                      const int *recvcounts,
                                                                      const int *displs,
                                                                      MPI_Datatype recvtype,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t * errflag,
                                                                      const
                                                                      MPIDI_coll_algo_container_t
                                                                      *
                                                                      ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *allgatherv_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype,
                                recvbuf, recvcounts, displs,
                                recvtype, comm_ptr, errflag, allgatherv_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgatherv_inter_composition_alpha(const void *sendbuf,
                                                                      int sendcount,
                                                                      MPI_Datatype sendtype,
                                                                      void *recvbuf,
                                                                      const int *recvcounts,
                                                                      const int *displs,
                                                                      MPI_Datatype recvtype,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t * errflag,
                                                                      const
                                                                      MPIDI_coll_algo_container_t
                                                                      *
                                                                      ch4_algo_parameters_container
                                                                      ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgatherv_inter_auto(sendbuf, sendcount, sendtype,
                                           recvbuf, recvcounts, displs,
                                           recvtype, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gather_intra_composition_alpha(const void *sendbuf,
                                                                  int sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, int recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  int root, MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *gather_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm, errflag, gather_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gather_inter_composition_alpha(const void *sendbuf,
                                                                  int sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, int recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  int root, MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  *
                                                                  ch4_algo_parameters_container
                                                                  ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gather_inter_auto(sendbuf, sendcount, sendtype, recvbuf,
                                       recvcount, recvtype, root, comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gatherv_intra_composition_alpha(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const int *recvcounts,
                                                                   const int *displs,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *gatherv_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                             displs, recvtype, root, comm, errflag, gatherv_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gatherv_inter_composition_alpha(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const int *recvcounts,
                                                                   const int *displs,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   *
                                                                   ch4_algo_parameters_container
                                                                   ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Gatherv_inter_auto(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcounts, displs, recvtype, root, comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatter_intra_composition_alpha(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   int recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *scatter_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, errflag, scatter_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatter_inter_composition_alpha(const void *sendbuf,
                                                                   int sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   int recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   int root,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag,
                                                                   const
                                                                   MPIDI_coll_algo_container_t
                                                                   *
                                                                   ch4_algo_parameters_container
                                                                   ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatter_inter_auto(sendbuf, sendcount, sendtype, recvbuf,
                                        recvcount, recvtype, root, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatterv_intra_composition_alpha(const void *sendbuf,
                                                                    const int *sendcounts,
                                                                    const int *displs,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    int recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    int root, MPIR_Comm * comm,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_coll_algo_container_t
                                                                    * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *scatterv_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag, scatterv_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatterv_inter_composition_alpha(const void *sendbuf,
                                                                    const int *sendcounts,
                                                                    const int *displs,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    int recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    int root, MPIR_Comm * comm,
                                                                    MPIR_Errflag_t * errflag,
                                                                    const
                                                                    MPIDI_coll_algo_container_t
                                                                    *
                                                                    ch4_algo_parameters_container
                                                                    ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Scatterv_inter_auto(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                         recvcount, recvtype, root, comm, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_inter_composition_alpha(const void *sendbuf,
                                                                          void *recvbuf,
                                                                          const int
                                                                          recvcounts[],
                                                                          MPI_Datatype
                                                                          datatype, MPI_Op op,
                                                                          MPIR_Comm * comm_ptr,
                                                                          MPIR_Errflag_t *
                                                                          errflag,
                                                                          const
                                                                          MPIDI_coll_algo_container_t
                                                                          *
                                                                          ch4_algo_parameters_container
                                                                          ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_inter_auto(sendbuf, recvbuf, recvcounts, datatype,
                                               op, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_intra_composition_alpha(const void *sendbuf,
                                                                          void *recvbuf,
                                                                          const int
                                                                          recvcounts[],
                                                                          MPI_Datatype
                                                                          datatype, MPI_Op op,
                                                                          MPIR_Comm * comm_ptr,
                                                                          MPIR_Errflag_t *
                                                                          errflag,
                                                                          const
                                                                          MPIDI_coll_algo_container_t
                                                                          *
                                                                          ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *reduce_scatter_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype,
                                    op, comm_ptr, errflag, reduce_scatter_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_block_inter_composition_alpha(const void
                                                                                *sendbuf,
                                                                                void *recvbuf,
                                                                                int recvcount,
                                                                                MPI_Datatype
                                                                                datatype,
                                                                                MPI_Op op,
                                                                                MPIR_Comm *
                                                                                comm_ptr,
                                                                                MPIR_Errflag_t
                                                                                * errflag,
                                                                                const
                                                                                MPIDI_coll_algo_container_t
                                                                                *
                                                                                ch4_algo_parameters_container
                                                                                ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Reduce_scatter_block_inter_auto(sendbuf, recvbuf, recvcount, datatype,
                                                     op, comm_ptr, errflag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_block_intra_composition_alpha(const void
                                                                                *sendbuf,
                                                                                void *recvbuf,
                                                                                int recvcount,
                                                                                MPI_Datatype
                                                                                datatype,
                                                                                MPI_Op op,
                                                                                MPIR_Comm *
                                                                                comm_ptr,
                                                                                MPIR_Errflag_t
                                                                                * errflag,
                                                                                const
                                                                                MPIDI_coll_algo_container_t
                                                                                *
                                                                                ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *reduce_scatter_block_container =
        MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                          op, comm_ptr, errflag, reduce_scatter_block_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan_intra_composition_alpha(const void *sendbuf,
                                                                void *recvbuf,
                                                                int count,
                                                                MPI_Datatype datatype,
                                                                MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag,
                                                                const
                                                                MPIDI_coll_algo_container_t
                                                                * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    MPI_Status status;
    void *tempbuf = NULL;
    void *localfulldata = NULL;
    void *prefulldata = NULL;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    int noneed = 1;             /* noneed=1 means no need to bcast tempbuf and
                                 * reduce tempbuf & recvbuf */
    MPIR_CHKLMEM_DECL(3);

    const void *scan_node_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
    const void *scan_roots_container = MPIDI_coll_get_next_container(scan_node_container);
    const void *bcast_node_container = MPIDI_coll_get_next_container(scan_roots_container);

    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_CHKLMEM_MALLOC(tempbuf, void *, count * (MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
    tempbuf = (void *) ((char *) tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_CHKLMEM_MALLOC(prefulldata, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan", MPL_MEM_BUFFER);
        prefulldata = (void *) ((char *) prefulldata - true_lb);

        if (comm_ptr->node_comm != NULL) {
            MPIR_CHKLMEM_MALLOC(localfulldata, void *, count * (MPL_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan", MPL_MEM_BUFFER);
            localfulldata = (void *) ((char *) localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_scan(sendbuf, recvbuf, count, datatype,
                               op, comm_ptr->node_comm, errflag, scan_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype,
                              op, comm_ptr->node_comm, errflag, scan_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    } else if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL) {
        mpi_errno = MPIC_Recv(localfulldata, count, datatype,
                              comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG,
                              comm_ptr->node_comm, &status, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else if (comm_ptr->node_roots_comm == NULL &&
               comm_ptr->node_comm != NULL &&
               MPIR_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1) {
        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                              0, MPIR_SCAN_TAG, comm_ptr->node_comm, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else if (comm_ptr->node_roots_comm != NULL) {
        localfulldata = recvbuf;
    }
    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is master
     * process of node 3. */
    if (comm_ptr->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_scan(localfulldata, prefulldata, count, datatype,
                              op, comm_ptr->node_roots_comm, errflag, scan_roots_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        if (MPIR_Get_internode_rank(comm_ptr, rank) != comm_ptr->node_roots_comm->local_size - 1) {
            mpi_errno = MPIC_Send(prefulldata, count, datatype,
                                  MPIR_Get_internode_rank(comm_ptr, rank) + 1,
                                  MPIR_SCAN_TAG, comm_ptr->node_roots_comm, errflag);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
        if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
            mpi_errno = MPIC_Recv(tempbuf, count, datatype,
                                  MPIR_Get_internode_rank(comm_ptr, rank) - 1,
                                  MPIR_SCAN_TAG, comm_ptr->node_roots_comm, &status, errflag);
            noneed = 0;
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if nessesary. */

    if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag,
                                bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#else
        mpi_errno =
            MPIDI_NM_mpi_bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag,
                               bcast_node_container);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    if (noneed == 0) {
        if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            mpi_errno =
                MPIDI_SHM_mpi_bcast(tempbuf, count, datatype, 0, comm_ptr->node_comm, errflag,
                                    bcast_node_container);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
#else
            mpi_errno =
                MPIDI_NM_mpi_bcast(tempbuf, count, datatype, 0, comm_ptr->node_comm, errflag,
                                   bcast_node_container);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        }

        mpi_errno = MPIR_Reduce_local(tempbuf, recvbuf, count, datatype, op);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan_intra_composition_beta(const void *sendbuf,
                                                               void *recvbuf,
                                                               int count,
                                                               MPI_Datatype datatype,
                                                               MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag,
                                                               const
                                                               MPIDI_coll_algo_container_t
                                                               * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *scan_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag, scan_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Exscan_intra_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  int count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag,
                                                                  const
                                                                  MPIDI_coll_algo_container_t
                                                                  * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    const void *exscan_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);

    mpi_errno =
        MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag,
                            exscan_container);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_COLL_IMPL_H_INCLUDED */
