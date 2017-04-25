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
#ifndef CH4_COLL_SELECT_H_INCLUDED
#define CH4_COLL_SELECT_H_INCLUDED

#include "ch4_impl.h"
#include "ch4r_proc.h"

#ifdef MPIDI_BUILD_CH4_SHM
#include "../shm/include/shm.h"
#endif

#include "../../mpi/coll/collutil.h"
#include "coll_algo_params.h"

/* NM_Bcast and SHM_Bcast */
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_alpha(void *buffer, int count, MPI_Datatype datatype,
                                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                           MPIDI_coll_algo_container_t *
                                                           ch4_algo_parameters_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_beta(void *buffer, int count, MPI_Datatype datatype,
                                                          int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                          MPIDI_coll_algo_container_t *
                                                          ch4_algo_parameters_container) MPL_STATIC_INLINE_SUFFIX;
/* NM_Bcast or SHM_Bcast */
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_gamma(void *buffer, int count, MPI_Datatype datatype,
                                                           int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                           MPIDI_coll_algo_container_t *
                                                           ch4_algo_parameters_container) MPL_STATIC_INLINE_SUFFIX;
/* SHM_Reduce, NM_Allreduce and SHM_Bcast */
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_composition_alpha(const void *sendbuf, void *recvbuf, int count,
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                               MPIDI_coll_algo_container_t *
                                                               ch4_algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
/* SHM_Reduce, NM_Reduce, NM_Bcast and SHM_Bcast */
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_composition_beta(const void *sendbuf, void *recvbuf, int count,
                                                              MPI_Datatype datatype, MPI_Op op,
                                                              MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                              MPIDI_coll_algo_container_t *
                                                              ch4_algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
/* NM_Reduce and NM_Bcast */
MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_composition_gamma(const void *sendbuf, void *recvbuf, int count,
                                                               MPI_Datatype datatype, MPI_Op op,
                                                               MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                               MPIDI_coll_algo_container_t *
                                                               ch4_algo_parameters_container)
    MPL_STATIC_INLINE_SUFFIX;
/* SHM_Reduce and NM_Reduce */
MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_composition_alpha(const void *sendbuf, void *recvbuf, int count,
                                                            MPI_Datatype datatype, MPI_Op op, int root,
                                                            MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                            MPIDI_coll_algo_container_t *
                                                            ch4_algo_parameters_container) MPL_STATIC_INLINE_SUFFIX;
/* NM_Reduce or SHM_Reduce */
MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_composition_beta(const void *sendbuf, void *recvbuf, int count,
                                                           MPI_Datatype datatype, MPI_Op op, int root,
                                                           MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                           MPIDI_coll_algo_container_t *
                                                           ch4_algo_parameters_container) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX MPIDI_coll_algo_container_t * MPIDI_CH4_Bcast_select(void *buffer,
                                                                              int count,
                                                                              MPI_Datatype datatype,
                                                                              int root,
                                                                              MPIR_Comm * comm,
                                                                              MPIR_Errflag_t *
                                                                              errflag)
{

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size;
    int nbytes=0, nbytes_homogeneous=0;
    int is_homogeneous;
    MPI_Aint type_size, type_size_homogeneous;

#ifdef MPID_HAS_HETERO
  if (comm_ptr->is_hetero)
      is_homogeneous = 0;
  else
      is_homogeneous = 1;
#endif
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

    nbytes = type_size * count;

    MPID_Datatype_get_size_macro(datatype, type_size_homogeneous);

    nbytes = MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE ? type_size*count : 0;
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BCAST &&
        nbytes <= MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE && MPIR_Comm_is_node_aware(comm)){
      if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm->local_size < MPIR_CVAR_BCAST_MIN_PROCS)){
        return (MPIDI_coll_algo_container_t *)&CH4_bcast_composition_alpha_cnt;
      }
      else{
        if(nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm->local_size, NULL)){
          return (MPIDI_coll_algo_container_t *)&CH4_bcast_composition_beta_cnt;
        }
      }
    }

    return (MPIDI_coll_algo_container_t *)&CH4_bcast_composition_gamma_cnt;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Errflag_t * errflag,
                                                  MPIDI_coll_algo_container_t *
                                                  ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_CH4_bcast_composition_alpha_id:
        mpi_errno =
            MPIDI_Bcast_composition_alpha(buffer, count, datatype, root, comm, errflag,
                                          ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_bcast_composition_beta_id:
        mpi_errno =
            MPIDI_Bcast_composition_beta(buffer, count, datatype, root, comm, errflag,
                                         ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_bcast_composition_gamma_id:
        mpi_errno =
            MPIDI_Bcast_composition_gamma(buffer, count, datatype, root, comm, errflag,
                                          ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_coll_algo_container_t * MPIDI_CH4_Allreduce_select(const void
                                                                                  *sendbuf,
                                                                                  void *recvbuf,
                                                                                  int count,
                                                                                  MPI_Datatype
                                                                                  datatype,
                                                                                  MPI_Op op,
                                                                                  MPIR_Comm * comm,
                                                                                  MPIR_Errflag_t *
                                                                                  errflag)
{
#ifdef MPID_HAS_HETERO
  int is_homogeneous;
  int rc;
#endif
  int comm_size, rank;
  MPI_Aint type_size;
  int mpi_errno = MPI_SUCCESS;
  int mpi_errno_ret = MPI_SUCCESS;
  int nbytes = 0;
  int mask, dst, is_commutative, pof2, newrank, rem, newdst, i,
      send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps; 
  MPI_Aint true_extent, true_lb, extent;
  void *tmp_buf;

  is_commutative = MPIR_Op_is_commutative(op);

  if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_ALLREDUCE) {
    /* is the op commutative? We do SMP optimizations only if it is. */
    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE ? type_size*count : 0;
    if (MPIR_Comm_is_node_aware(comm) && is_commutative &&
        nbytes <= MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE) {
        return (MPIDI_coll_algo_container_t *)&CH4_allreduce_composition_alpha_cnt;
    }
  }

#ifdef MPID_HAS_HETERO
  if (comm_ptr->is_hetero)
      is_homogeneous = 0;
  else
      is_homogeneous = 1;
#endif

#ifdef MPID_HAS_HETERO
  if (!is_homogeneous) {
    return (MPIDI_coll_algo_container_t *)&CH4_allreduce_composition_beta_cnt;
  }
  else
#endif /* MPID_HAS_HETERO */
  {
    return (MPIDI_coll_algo_container_t *)&CH4_allreduce_composition_gamma_cnt;
  }
  MPIR_Assert(0);
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                      MPIDI_coll_algo_container_t *
                                                      ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_CH4_allreduce_composition_alpha_id:
        mpi_errno =
            MPIDI_Allreduce_composition_alpha(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                              ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_allreduce_composition_beta_id:
        mpi_errno =
            MPIDI_Allreduce_composition_beta(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                             ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_allreduce_composition_gamma_id:
        mpi_errno =
            MPIDI_Allreduce_composition_gamma(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                              ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        break;
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_coll_algo_container_t * MPIDI_CH4_Reduce_select(const void *sendbuf,
                                                                               void *recvbuf,
                                                                               int count,
                                                                               MPI_Datatype datatype,
                                                                               MPI_Op op, int root,
                                                                               MPIR_Comm * comm,
                                                                               MPIR_Errflag_t *
                                                                               errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int comm_size, is_commutative, type_size, pof2;
    int nbytes = 0;
    MPIR_Op *op_ptr;

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_REDUCE) {
      /* is the op commutative? We do SMP optimizations only if it is. */
      if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN)
          is_commutative = 1;
      else {
          MPIR_Op_get_ptr(op, op_ptr);
          is_commutative = (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE) ? 0 : 1;
      }

      MPID_Datatype_get_size_macro(datatype, type_size);
      nbytes = MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE ? type_size*count : 0;
      if (MPIR_Comm_is_node_aware(comm) && is_commutative &&
          nbytes <= MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE) {
        return (MPIDI_coll_algo_container_t *)&CH4_reduce_composition_alpha_cnt;
      }
    }
    return (MPIDI_coll_algo_container_t *)&CH4_reduce_composition_beta_cnt;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   MPIDI_coll_algo_container_t *
                                                   ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_CH4_reduce_composition_alpha_id:
        mpi_errno =
            MPIDI_Reduce_composition_alpha(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                           ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_reduce_composition_beta_id:
        mpi_errno =
            MPIDI_Reduce_composition_beta(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                          ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
        break;

    }
    return mpi_errno;
}
#endif /* CH4_COLL_SELECT_H_INCLUDED */
