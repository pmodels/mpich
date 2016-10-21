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
#include "ch4_coll_params.h"
#include "coll_algo_params.h"

/* NM_Bcast and SHM_Bcast */
MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_composition_alpha(void *buffer, int count, MPI_Datatype datatype,
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

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Bcast_knomial(void *buffer,
                                                     int count,
                                                     MPI_Datatype datatype,
                                                     int root,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag,
                                                     MPIDI_coll_algo_container_t * params_container)
    MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_collective_selection_init(MPIR_Comm * comm)
{

    int coll_id;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = NULL;

    tuner_table_ptr =
        (MPIDI_coll_tuner_table_t *) MPL_malloc(MPIDI_NUM_COLLECTIVES *
                                                sizeof(MPIDI_coll_tuner_table_t));

    /* initializing tuner_table_ptr for each collective */
    for (coll_id = 0; coll_id < MPIDI_NUM_COLLECTIVES; coll_id++) {
        tuner_table_ptr[coll_id].table_size = MPIDI_coll_table_size[CH4][coll_id];

        if (tuner_table_ptr[coll_id].table_size) {
            tuner_table_ptr[coll_id].table = MPIDI_coll_tuning_table[CH4][coll_id];
        }
    }

    MPIDI_CH4_COMM(comm).colltuner_table = tuner_table_ptr;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_collective_selection_free(MPIR_Comm * comm)
{

    MPIDI_coll_tuner_table_t *tuner_table_ptr = NULL;

    tuner_table_ptr = MPIDI_CH4_COMM(comm).colltuner_table;

    if (tuner_table_ptr)
        MPL_free(tuner_table_ptr);
}

MPL_STATIC_INLINE_PREFIX MPIDI_coll_algo_container_t *MPIDI_CH4_Bcast_select(void *buffer,
                                                                                 int count,
                                                                                 MPI_Datatype datatype,
                                                                                 int root,
                                                                                 MPIR_Comm * comm,
                                                                                 MPIR_Errflag_t *
                                                                                 errflag)
{

if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM || comm->hierarchy_kind != MPIR_COMM_HIERARCHY_KIND__PARENT){
    mpir_fallback_container.id = MPIDI_CH4_COLL_AUTO_SELECT;
    memset(&(mpir_fallback_container.params), 0x0, sizeof(MPIDI_COLL_ALGO_params_t));
    return &mpir_fallback_container;
}

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    MPIDI_coll_algo_container_t *shm_exclusive_algo_parameters_container;
    shm_exclusive_algo_parameters_container = MPL_malloc(sizeof(MPIDI_coll_algo_container_t));
#ifdef MPIDI_BUILD_CH4_SHM
    shm_exclusive_algo_parameters_container->id = MPIDI_POSIX_bcast_empty_id;
#else /* MPIDI_BUILD_CH4_SHM */
    shm_exclusive_algo_parameters_container->id = 0;
#endif /* MPIDI_BUILD_CH4_SHM */
    memset(&(shm_exclusive_algo_parameters_container->params), 0x0, sizeof(MPIDI_COLL_ALGO_params_t));
    return shm_exclusive_algo_parameters_container;
#else /* MPIDI_CH4_EXCLUSIVE_SHM */
    MPI_Aint type_size;
    int nbytes;
    int i = 0;

    MPIDI_coll_tuner_table_t *tuning_table_ptr = &MPIDI_CH4_COMM(comm).colltuner_table[MPIDI_BCAST];
    MPIDI_coll_tuning_table_entry_t *entry = tuning_table_ptr->table;

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    while (nbytes > entry[i].msg_size && entry[i].msg_size != -1 && i < tuning_table_ptr->table_size) {
        i++;
    }

    return (MPIDI_coll_algo_container_t *)entry[i].algo_container;
#endif /* MPIDI_CH4_EXCLUSIVE_SHM */
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Errflag_t * errflag,
                                                  MPIDI_coll_algo_container_t *
                                                  ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * next_container = NULL;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_CH4_bcast_composition_nm_id:
        next_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
        mpi_errno =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag,
                               next_container);
        break;
#ifdef MPIDI_BUILD_CH4_SHM        
    case MPIDI_CH4_bcast_composition_shm_id:
        next_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);
        mpi_errno =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, root, comm, errflag,
                                next_container);
        break;
    case MPIDI_CH4_bcast_composition_alpha_id:
        mpi_errno =
            MPIDI_Bcast_composition_alpha(buffer, count, datatype, root, comm, errflag,
                                          ch4_algo_parameters_container);
        break;
#endif
    default:
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_coll_algo_container_t *MPIDI_CH4_Allreduce_select(const void
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

int is_commutative;

is_commutative = MPIR_Op_is_commutative(op);

if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM || comm->hierarchy_kind != MPIR_COMM_HIERARCHY_KIND__PARENT || !is_commutative){
    mpir_fallback_container.id = MPIDI_CH4_COLL_AUTO_SELECT;
    memset(&(mpir_fallback_container.params), 0x0, sizeof(MPIDI_COLL_ALGO_params_t));
    return &mpir_fallback_container;
}

#ifndef MPIDI_CH4_EXCLUSIVE_SHM
#ifdef MPIDI_BUILD_CH4_SHM
    mpir_fallback_container.id = MPIDI_POSIX_allreduce_empty_id;
#else
    mpir_fallback_container.id = 0;
#endif
    memset(&(mpir_fallback_container.params), 0x0, sizeof(MPIDI_COLL_ALGO_params_t));
    return &mpir_fallback_container;
#else
    MPI_Aint type_size;
    int nbytes;
    int i = 0;

    MPIDI_coll_tuner_table_t *tuning_table_ptr =
        &MPIDI_CH4_COMM(comm).colltuner_table[MPIDI_ALLREDUCE];
    MPIDI_coll_tuning_table_entry_t *entry = tuning_table_ptr->table;

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    while (nbytes > entry[i].msg_size && entry[i].msg_size != -1 && i < tuning_table_ptr->table_size) {
        i++;
    }

    return (MPIDI_coll_algo_container_t *)entry[i].algo_container;
#endif
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                      MPIDI_coll_algo_container_t *
                                                      ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    void * next_container;

    switch (ch4_algo_parameters_container->id) {
#ifdef MPIDI_BUILD_CH4_SHM        
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
#endif
    case MPIDI_CH4_allreduce_composition_gamma_id:
        mpi_errno =
            MPIDI_Allreduce_composition_gamma(sendbuf, recvbuf, count, datatype, op, comm, errflag,
                                              ch4_algo_parameters_container);
        break;
    case MPIDI_CH4_allreduce_composition_nm_id:
        next_container = MPIDI_coll_get_next_container(ch4_algo_parameters_container);    
        mpi_errno =
            MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm->node_roots_comm,
                                   errflag, next_container);
        break;
    default:
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
        break;
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_coll_algo_container_t *MPIDI_CH4_Reduce_select(const void *sendbuf,
                                                                                  void *recvbuf,
                                                                                  int count,
                                                                                  MPI_Datatype datatype,
                                                                                  MPI_Op op, int root,
                                                                                  MPIR_Comm * comm,
                                                                                  MPIR_Errflag_t *
                                                                                  errflag)
{

int is_commutative;

is_commutative = MPIR_Op_is_commutative(op);

if (comm->comm_kind == MPIR_COMM_KIND__INTERCOMM || comm->hierarchy_kind != MPIR_COMM_HIERARCHY_KIND__PARENT || !is_commutative){
    mpir_fallback_container.id = MPIDI_CH4_COLL_AUTO_SELECT;
    memset(&(mpir_fallback_container.params), 0x0, sizeof(MPIDI_COLL_ALGO_params_t));
    return &mpir_fallback_container;
}

    MPI_Aint type_size;
    int nbytes;
    int i = 0;

    MPIDI_coll_tuner_table_t *tuning_table_ptr =
        &MPIDI_CH4_COMM(comm).colltuner_table[MPIDI_REDUCE];
    MPIDI_coll_tuning_table_entry_t *entry = tuning_table_ptr->table;


    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    while (nbytes > entry[i].msg_size && entry[i].msg_size != -1 && i < tuning_table_ptr->table_size) {
        i++;
    }

    return (MPIDI_coll_algo_container_t *)entry[i].algo_container;
}


MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root,
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag,
                                                   MPIDI_coll_algo_container_t *
                                                   ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_coll_algo_container_t * next_container;

    switch (-1) {
    case MPIDI_CH4_reduce_composition_alpha_id:
        mpi_errno =
            MPIDI_Reduce_composition_alpha(sendbuf, recvbuf, count, datatype, op, root, comm, errflag,
                                           ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
        break;

    }
    return mpi_errno;
}
#endif /* CH4_COLL_SELECT_H_INCLUDED */
