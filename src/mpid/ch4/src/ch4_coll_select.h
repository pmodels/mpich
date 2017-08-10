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
#include "ch4_coll_impl.h"

#ifdef MPIDI_BUILD_CH4_SHM
#include "../shm/include/shm.h"
#endif

#include "../../mpi/coll/collutil.h"

MPL_STATIC_INLINE_PREFIX
MPIDI_coll_algo_container_t *MPIDI_CH4_Barrier_select(MPIR_Comm * comm,
                                                      MPIR_Errflag_t * errflag)
{
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) &CH4_barrier_intercomm_cnt;
    }
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BARRIER &&
        MPIR_Comm_is_node_aware(comm)) {
        return (MPIDI_coll_algo_container_t *) &CH4_barrier_composition_alpha_cnt;
    }

    return (MPIDI_coll_algo_container_t *) &CH4_barrier_composition_beta_cnt;
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_CH4_Barrier_call(MPIR_Comm * comm,
                           MPIR_Errflag_t * errflag,
                           MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_CH4_barrier_composition_alpha_id:
        mpi_errno = MPIDI_Barrier_composition_alpha(comm, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_barrier_composition_beta_id:
        mpi_errno = MPIDI_Barrier_composition_beta(comm, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_barrier_intercomm_id:
        mpi_errno = MPIDI_Barrier_intercomm(comm, errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Barrier(comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_coll_algo_container_t *MPIDI_CH4_Bcast_select(void *buffer,
                                                    int count,
                                                    MPI_Datatype datatype,
                                                    int root,
                                                    MPIR_Comm * comm,
                                                    MPIR_Errflag_t * errflag)
{
    int nbytes = 0;
    MPI_Aint type_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) &CH4_bcast_intercomm_cnt;
    }
    nbytes = MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE ? type_size * count : 0;
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BCAST &&
        nbytes <= MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE && MPIR_Comm_is_node_aware(comm)) {
        if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
            (comm->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
            return (MPIDI_coll_algo_container_t *) &CH4_bcast_composition_alpha_cnt;
        } else {
            if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm->local_size, NULL)) {
                return (MPIDI_coll_algo_container_t *) &CH4_bcast_composition_beta_cnt;
            }
        }
    }
    return (MPIDI_coll_algo_container_t *) &CH4_bcast_composition_gamma_cnt;
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_CH4_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                         int root, MPIR_Comm * comm,
                         MPIR_Errflag_t * errflag,
                         MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
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
    case MPIDI_CH4_bcast_intercomm_id:
        mpi_errno =
            MPIDI_Bcast_intercomm(buffer, count, datatype, root, comm, errflag,
                                  ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_coll_algo_container_t *MPIDI_CH4_Allreduce_select(const void *sendbuf,
                                                        void *recvbuf,
                                                        int count,
                                                        MPI_Datatype
                                                        datatype,
                                                        MPI_Op op,
                                                        MPIR_Comm * comm,
                                                        MPIR_Errflag_t * errflag)
{
    MPI_Aint type_size;
    int nbytes = 0;
    int is_commutative;

    is_commutative = MPIR_Op_is_commutative(op);
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) &CH4_allreduce_intercomm_cnt;
    }

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_ALLREDUCE) {
        /* is the op commutative? We do SMP optimizations only if it is. */
        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE ? type_size * count : 0;
        if (MPIR_Comm_is_node_aware(comm) && is_commutative &&
            nbytes <= MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE) {
            return (MPIDI_coll_algo_container_t *) &CH4_allreduce_composition_alpha_cnt;
        }
    }
    return (MPIDI_coll_algo_container_t *) &CH4_allreduce_composition_beta_cnt;
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_CH4_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op,
                             MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                             MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
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
    case MPIDI_CH4_allreduce_intercomm_id:
        mpi_errno =
            MPIDI_Allreduce_intercomm(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                      ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        break;
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_coll_algo_container_t *MPIDI_CH4_Reduce_select(const void *sendbuf,
                                                     void *recvbuf,
                                                     int count,
                                                     MPI_Datatype datatype,
                                                     MPI_Op op, int root,
                                                     MPIR_Comm * comm,
                                                     MPIR_Errflag_t * errflag)
{
    int is_commutative, type_size;
    int nbytes = 0;
    MPIR_Op *op_ptr;

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) &CH4_reduce_intercomm_cnt;
    }
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_REDUCE) {
        /* is the op commutative? We do SMP optimizations only if it is. */
        if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
            is_commutative = 1;
        } else {
            MPIR_Op_get_ptr(op, op_ptr);
            is_commutative = (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE) ? 0 : 1;
        }

        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE ? type_size * count : 0;
        if (MPIR_Comm_is_node_aware(comm) && is_commutative &&
            nbytes <= MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE) {
            return (MPIDI_coll_algo_container_t *) &CH4_reduce_composition_alpha_cnt;
        }
    }
    return (MPIDI_coll_algo_container_t *) &CH4_reduce_composition_beta_cnt;
}


MPL_STATIC_INLINE_PREFIX
int MPIDI_CH4_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                          MPI_Datatype datatype, MPI_Op op, int root,
                          MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                          MPIDI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_CH4_reduce_composition_alpha_id:
        mpi_errno =
            MPIDI_Reduce_composition_alpha(sendbuf, recvbuf, count, datatype, op, root, comm,
                                           errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_reduce_composition_beta_id:
        mpi_errno =
            MPIDI_Reduce_composition_beta(sendbuf, recvbuf, count, datatype, op, root, comm,
                                          errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_reduce_intercomm_id:
        mpi_errno =
            MPIDI_Reduce_intercomm(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                   ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
        break;
    }
    return mpi_errno;
}
#endif /* CH4_COLL_SELECT_H_INCLUDED */
