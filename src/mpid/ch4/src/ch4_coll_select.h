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
#include "ch4_coll.h"
#include "ch4_coll_params.h"

static inline int MPIDI_collective_selection_init(MPIR_Comm * comm)
{

    int i, coll_id;
    MPIDI_coll_tuner_table_t *tuner_table_ptr;

    tuner_table_ptr = (MPIDI_coll_tuner_table_t *)MPL_malloc(MPIDI_NUM_COLLECTIVES*sizeof(MPIDI_coll_tuner_table_t));

    /* initializing tuner_table_ptr for each collective */
    for( coll_id = 0; coll_id < MPIDI_NUM_COLLECTIVES; coll_id++ )
    {
        tuner_table_ptr[coll_id].table_size = table_size[CH4][coll_id];
        tuner_table_ptr[coll_id].table = (MPIDI_coll_table_entry_t **)MPL_malloc(tuner_table_ptr[coll_id].table_size*sizeof(MPIDI_coll_table_entry_t*));
        if(tuner_table_ptr[coll_id].table)
        {
            for( i = 0; i < tuner_table_ptr[coll_id].table_size; i++)
            {
                tuner_table_ptr[coll_id].table[i] = &tuning_table[CH4][coll_id][i];
            }
        }
    }

    MPIDI_CH4_COMM(comm).colltuner_table = tuner_table_ptr;
}

static inline int MPIDI_collective_selection_free(MPIR_Comm * comm)
{

    int i, coll_id;
    MPIDI_coll_tuner_table_t *tuner_table_ptr;

    tuner_table_ptr = MPIDI_CH4_COMM(comm).colltuner_table;

    for( i = 0; i < MPIDI_NUM_COLLECTIVES; i++ )
    {
        if(tuner_table_ptr[i].table);
        {
            MPL_free(tuner_table_ptr[i].table);
        }
    }
    if(tuner_table_ptr)
        MPL_free(tuner_table_ptr);
}



static inline int MPIDI_CH4_Bcast_select(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag, 
                                         MPIDI_algo_parameters_t **algo_parameters_ptr)
{
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    return  0;
#else
    //find pointer to table_entry, fill *algo_parameters_ptr, return algorithm id
    int i;
    MPI_Aint type_size;
    int nbytes;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = &MPIDI_CH4_COMM(comm).colltuner_table[MPIDI_BCAST];

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size*count;

    for(i = 0; i < tuner_table_ptr->table_size; i++) 
    {
        if(tuner_table_ptr->table[i]->msg_size < nbytes)
        {
            *algo_parameters_ptr = &(tuner_table_ptr->table[i]->params);
            return tuner_table_ptr->table[i]->algo_id;
        }
    }
    return 2;
#endif
}

static inline int MPIDI_CH4_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                       int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag, 
                                       int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 0:
            mpi_errno = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
        case 1:
            mpi_errno = MPIDI_SHM_mpi_bcast(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
        case 2:
            mpi_errno = MPIDI_Bcast_topo(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
        default:
            mpi_errno = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
    }

    return mpi_errno;
}

static inline int MPIDI_CH4_Allreduce_select(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                         MPIR_Errflag_t * errflag, MPIDI_algo_parameters_t **algo_parameters_ptr)
{
#ifndef MPIDI_CH4_EXCLUSIVE_SHM
    return  3;
#else
    int i;
    MPI_Aint type_size;
    int nbytes;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = &MPIDI_CH4_COMM(comm).colltuner_table[MPIDI_ALLREDUCE];

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size*count;

    for(i = 0; i < tuner_table_ptr->table_size; i++) 
    {
        if(tuner_table_ptr->table[i]->msg_size < nbytes)
        {
            *algo_parameters_ptr = &(tuner_table_ptr->table[i]->params);
            return tuner_table_ptr->table[i]->algo_id;
        }
    }
    return 0;
#endif
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                                      MPIR_Errflag_t * errflag, 
                                                      int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    switch(algo_number)
    {
        case 0:
            mpi_errno = MPIDI_Allreduce_0(sendbuf, recvbuf, count, datatype, op, comm, errflag, ch4_algo_parameters_ptr);
            break;
        case 1:
            mpi_errno = MPIDI_Allreduce_1(sendbuf, recvbuf, count, datatype, op, comm, errflag, ch4_algo_parameters_ptr);
            break;
        case 2:
            mpi_errno = MPIDI_Allreduce_2(sendbuf, recvbuf, count, datatype, op, comm, errflag, ch4_algo_parameters_ptr);
            break;
        case 3:
            mpi_errno = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm->node_roots_comm, errflag, ch4_algo_parameters_ptr);
            break;
        default:
            mpi_errno = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag, ch4_algo_parameters_ptr);
            break;
    }
    return mpi_errno;
}


static inline int MPIDI_CH4_Reduce_select(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm,
                                          MPIR_Errflag_t * errflag, MPIDI_algo_parameters_t **algo_parameters_ptr)
{
    int i;
    MPI_Aint type_size;
    int nbytes;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = &MPIDI_CH4_COMM(comm).colltuner_table[MPIDI_REDUCE];

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size*count;

    for(i = 0; i < tuner_table_ptr->table_size; i++) 
    {
        if(tuner_table_ptr->table[i]->msg_size < nbytes)
        {
            *algo_parameters_ptr = &(tuner_table_ptr->table[i]->params);
            return tuner_table_ptr->table[i]->algo_id;
        }
    }
    return 0;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_CH4_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root, 
                                                   MPIR_Comm * comm, MPIR_Errflag_t * errflag, 
                                                   int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    switch(algo_number)
    {
        case 0:
            mpi_errno = MPIDI_Reduce_0(sendbuf, recvbuf, count, datatype, op, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
        default:
            mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
    }
    return mpi_errno;
}
#endif /* CH4_COLL_SELECT_H_INCLUDED */