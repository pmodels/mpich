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


MPL_STATIC_INLINE_PREFIX
    MPIDI_coll_algo_container_t * MPIDI_CH4_Barrier_select(MPIR_Comm * comm,
                                                           MPIR_Errflag_t * errflag)
{
    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) & CH4_Barrier_inter_composition_alpha_cnt;
    }
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BARRIER &&
        MPIR_Comm_is_node_aware(comm)) {
        return (MPIDI_coll_algo_container_t *) & CH4_Barrier_intra_composition_alpha_cnt;
    }

    return (MPIDI_coll_algo_container_t *) & CH4_Barrier_intra_composition_beta_cnt;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_coll_algo_container_t * MPIDI_CH4_Bcast_select(void *buffer,
                                                         int count,
                                                         MPI_Datatype datatype,
                                                         int root,
                                                         MPIR_Comm * comm, MPIR_Errflag_t * errflag)
{
    int nbytes = 0;
    MPI_Aint type_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) & CH4_Bcast_inter_composition_alpha_cnt;
    }
    nbytes = MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE ? type_size * count : 0;
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_BCAST &&
        nbytes <= MPIR_CVAR_MAX_SMP_BCAST_MSG_SIZE && MPIR_Comm_is_node_aware(comm)) {
        if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
            (comm->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
            return (MPIDI_coll_algo_container_t *) & CH4_Bcast_intra_composition_alpha_cnt;
        } else {
            if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(comm->local_size, NULL)) {
                return (MPIDI_coll_algo_container_t *) & CH4_Bcast_intra_composition_beta_cnt;
            }
        }
    }
    return (MPIDI_coll_algo_container_t *) & CH4_Bcast_intra_composition_gamma_cnt;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_coll_algo_container_t * MPIDI_CH4_Allreduce_select(const void *sendbuf,
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
        return (MPIDI_coll_algo_container_t *) & CH4_Allreduce_inter_composition_alpha_cnt;
    }

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_ALLREDUCE) {
        /* is the op commutative? We do SMP optimizations only if it is. */
        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE ? type_size * count : 0;
        if (MPIR_Comm_is_node_aware(comm) && is_commutative &&
            nbytes <= MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE) {
            return (MPIDI_coll_algo_container_t *) & CH4_Allreduce_intra_composition_alpha_cnt;
        }
    }
    return (MPIDI_coll_algo_container_t *) & CH4_Allreduce_intra_composition_beta_cnt;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_coll_algo_container_t * MPIDI_CH4_Reduce_select(const void *sendbuf,
                                                          void *recvbuf,
                                                          int count,
                                                          MPI_Datatype datatype,
                                                          MPI_Op op, int root,
                                                          MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag)
{
    int is_commutative, type_size;
    int nbytes = 0;

    if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        return (MPIDI_coll_algo_container_t *) & CH4_Reduce_inter_composition_alpha_cnt;
    }
    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_REDUCE) {
        /* is the op commutative? We do SMP optimizations only if it is. */
        is_commutative = MPIR_Op_is_commutative(op);

        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE ? type_size * count : 0;
        if (MPIR_Comm_is_node_aware(comm) && is_commutative &&
            nbytes <= MPIR_CVAR_MAX_SMP_REDUCE_MSG_SIZE) {
            return (MPIDI_coll_algo_container_t *) & CH4_Reduce_intra_composition_alpha_cnt;
        }
    }
    return (MPIDI_coll_algo_container_t *) & CH4_Reduce_intra_composition_beta_cnt;
}

#endif /* CH4_COLL_SELECT_H_INCLUDED */
