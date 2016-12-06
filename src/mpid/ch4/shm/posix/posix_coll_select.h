#ifndef SHM_POSIX_COLL_SELECT_H_INCLUDED
#define SHM_POSIX_COLL_SELECT_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"

static inline int MPIDI_SHM_collective_selection_init(MPIR_Comm * comm)
{
    int i, coll_id;
    MPIDI_coll_params_t *coll_params;

    coll_params = (MPIDI_coll_params_t *)MPL_malloc(MPIDI_COLLECTIVE_NUMBER*sizeof(MPIDI_coll_params_t));

    for( coll_id = 0; coll_id < MPIDI_COLLECTIVE_NUMBER; coll_id++ )
    {
        coll_params[coll_id].table_size = table_size[SHM][coll_id];
        coll_params[coll_id].table      = (MPIDI_table_entry_t **)MPL_malloc(coll_params[coll_id].table_size*sizeof(MPIDI_table_entry_t*));
        if(coll_params[coll_id].table)
        {
            for( i = 0; i <  coll_params[coll_id].table_size; i++)
            {
                coll_params[coll_id].table[i] = &tuning_table[SHM][coll_id][i];
            }
        }
        MPIDI_POSIX_COMM(comm).coll_params = coll_params;
    }
}

static inline int MPIDI_SHM_collective_selection_free(MPIR_Comm * comm)
{

    int i, coll_id;
    MPIDI_coll_params_t* coll_params;

    coll_params = (MPIDI_coll_params_t *)MPIDI_POSIX_COMM(comm).coll_params;

    for( i = 0; i < MPIDI_COLLECTIVE_NUMBER; i++ )
    {
        if(coll_params[i].table)
        {
            MPL_free(coll_params[i].table);
        }
    }
    if(coll_params)
        MPL_free(coll_params);
}

static inline int MPIDI_SHM_Bcast_select(void *buffer, int count, MPI_Datatype datatype,
                                         int root, MPIDI_coll_params_t * coll_params, MPIR_Errflag_t * errflag,
                                         MPIDI_algo_parameters_t *ch4_algo_parameters_ptr_in,
                                         MPIDI_algo_parameters_t **shm_algo_parameters_ptr_out)
{
    //find pointer to table_entry, fill algo_parameters_ptr, return algorithm id
    int i;
    MPI_Aint type_size;
    int nbytes;

    if(ch4_algo_parameters_ptr_in->ch4_bcast.shm_bcast == -1)
    {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size*count;

        for(i = 0; i < coll_params->table_size; i++) 
        {
            if(coll_params->table[i]->threshold < nbytes)
            {
                *shm_algo_parameters_ptr_out = &(coll_params->table[i]->params);
                return coll_params->table[i]->algo_id;
            }
        }
    }
    else
    {
        *shm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_SHM_bcast_param_defaults[ch4_algo_parameters_ptr_in->ch4_bcast.shm_bcast];
        return ch4_algo_parameters_ptr_in->ch4_bcast.shm_bcast;
    }
    return 0;
}

static inline int MPIDI_SHM_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                       int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag, 
                                       int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 3:
            mpi_errno = MPIDI_SHM_Bcast_knomial(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_ptr);
            break;
        default:
            mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
            break;
    }

    return mpi_errno;
}

static inline int MPIDI_SHM_Allreduce_select(const void *sendbuf, void *recvbuf, int count,
                                             MPI_Datatype datatype, MPI_Op op, MPIDI_coll_params_t * coll_params,
                                             MPIR_Errflag_t * errflag, 
                                             MPIDI_algo_parameters_t *ch4_algo_parameters_ptr_in,
                                             MPIDI_algo_parameters_t **shm_algo_parameters_ptr_out)
{
    //find pointer to table_entry, fill algo_parameters_ptr, return algorithm id
    int i;
    MPI_Aint type_size;
    int nbytes;

    if(ch4_algo_parameters_ptr_in->ch4_bcast.shm_bcast == -1)
    { 
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size*count;

        for(i = 0; i < coll_params->table_size; i++) 
        {
            if(coll_params->table[i]->threshold < nbytes)
            {
                *shm_algo_parameters_ptr_out = &(coll_params->table[i]->params);
                return coll_params->table[i]->algo_id;
            }
        }
    }
    else
    {
        *shm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_SHM_reduce_param_defaults[ch4_algo_parameters_ptr_in->ch4_allreduce.shm_allreduce];
        return ch4_algo_parameters_ptr_in->ch4_allreduce.shm_allreduce;     
    }
    return 0;
}


static inline int MPIDI_SHM_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag,
                                           int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 5:
            mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        case 7:
            mpi_errno = MPIDI_SHM_Allreduce_1(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag, ch4_algo_parameters_ptr);
            break;
        default:
            mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;
    }

    return mpi_errno;
}


static inline int MPIDI_SHM_Reduce_select(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, int root, MPIDI_coll_params_t * coll_params,
                                          MPIR_Errflag_t * errflag, 
                                          MPIDI_algo_parameters_t *ch4_algo_parameters_ptr_in,
                                          MPIDI_algo_parameters_t **shm_algo_parameters_ptr_out)
{
    //find pointer to table_entry, fill algo_parameters_ptr, return algorithm id
    int i;
    MPI_Aint type_size;
    int nbytes;

    if(ch4_algo_parameters_ptr_in->ch4_bcast.shm_bcast == -1)
    {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size*count;

        for(i = 0; i < coll_params->table_size; i++) 
        {
            if(coll_params->table[i]->threshold < nbytes)
            {
                *shm_algo_parameters_ptr_out = &(coll_params->table[i]->params);
                return coll_params->table[i]->algo_id;
            }
        }
    }
    else
    {
        *shm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_SHM_reduce_param_defaults[ch4_algo_parameters_ptr_in->ch4_reduce.shm_reduce];
        return ch4_algo_parameters_ptr_in->ch4_reduce.shm_reduce;  
    }
    return 0;
}

static inline int MPIDI_SHM_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                        MPIR_Errflag_t * errflag, int algo_number, 
                                        MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 11:
            mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        case 23:
            mpi_errno = MPIDI_SHM_Reduce_1(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag, ch4_algo_parameters_ptr);
            break;
        default:
            mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
            break;
    }

    return mpi_errno;
}
#endif /* SHM_POSIX_COLL_SELECT_H_INCLUDED */