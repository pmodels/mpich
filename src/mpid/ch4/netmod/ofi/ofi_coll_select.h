#ifndef NETMOD_OFI_COLL_SELECT_H_INCLUDED
#define NETMOD_OFI_COLL_SELECT_H_INCLUDED

#include "ofi_impl.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"

static inline int MPIDI_NM_collective_selection_init(MPIR_Comm * comm)
{
    int i, coll_id;
    MPIDI_coll_tuner_table_t *tuner_table_ptr;

    tuner_table_ptr = (MPIDI_coll_tuner_table_t *)MPL_malloc(MPIDI_NUM_COLLECTIVES*sizeof(MPIDI_coll_tuner_table_t));

    for( coll_id = 0; coll_id < MPIDI_NUM_COLLECTIVES; coll_id++ )
    {
        tuner_table_ptr[coll_id].table_size = table_size[NETMOD][coll_id];
        tuner_table_ptr[coll_id].table      = (MPIDI_coll_table_entry_t **)MPL_malloc(tuner_table_ptr[coll_id].table_size*sizeof(MPIDI_coll_table_entry_t*));
        if(tuner_table_ptr[coll_id].table)
        {
            for( i = 0; i < tuner_table_ptr[coll_id].table_size; i++)
            {
                tuner_table_ptr[coll_id].table[i] = &tuning_table[NETMOD][coll_id][i];
            }
        }
        MPIDI_OFI_COMM(comm).colltuner_table = tuner_table_ptr;
    }
}


static inline int MPIDI_NM_collective_selection_free(MPIR_Comm * comm)
{
    int i, coll_id;
    MPIDI_coll_tuner_table_t* tuner_table_ptr;

    tuner_table_ptr = MPIDI_OFI_COMM(comm).colltuner_table;

    for( i = 0; i < MPIDI_NUM_COLLECTIVES; i++ )
    {
        if(tuner_table_ptr[i].table)
        {
            MPL_free(tuner_table_ptr[i].table);
        }
    }
    if(tuner_table_ptr)
        MPL_free(tuner_table_ptr);
}


static inline int MPIDI_NM_Bcast_select(void *buffer, int count, MPI_Datatype datatype,
                                        int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                        MPIDI_algo_parameters_t *ch4_algo_parameters_ptr_in,
                                        MPIDI_algo_parameters_t **nm_algo_parameters_ptr_out)
{
    int i;
    MPI_Aint type_size;
    int nbytes;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = &MPIDI_OFI_COMM(comm_ptr).colltuner_table[MPIDI_BCAST];

    if(ch4_algo_parameters_ptr_in->ch4_bcast.nm_bcast == -1 )
    {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size*count;

        for(i = 0; i < tuner_table_ptr->table_size; i++) 
        {
            if(tuner_table_ptr->table[i]->msg_size < nbytes)
            {
                continue;
            }
            else
            {
                *nm_algo_parameters_ptr_out = &(tuner_table_ptr->table[i]->params);
                return tuner_table_ptr->table[i]->algo_id;
            }
        }
    }
    else
    {
        *nm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_CH4_bcast_ofi_param_defaults[ch4_algo_parameters_ptr_in->ch4_bcast.nm_bcast];
        return ch4_algo_parameters_ptr_in->ch4_bcast.nm_bcast;

    }

    //default parameters and algo_id
    *nm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_CH4_bcast_ofi_param_defaults[tuner_table_ptr->table[0]->algo_id];
    return tuner_table_ptr->table[0]->algo_id;
}

static inline int MPIDI_NM_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                      int root, MPIR_Comm * comm, MPIR_Errflag_t * errflag, 
                                      int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 1:
            mpi_errno = MPIDI_NM_Bcast_knomial(buffer, count, datatype, root, comm, errflag, ch4_algo_parameters_ptr); 
            break;
        default:
            mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
            break;
    }

    return mpi_errno;
}

static inline int MPIDI_NM_Allreduce_select(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                        MPIR_Errflag_t * errflag, 
                                        MPIDI_algo_parameters_t *ch4_algo_parameters_ptr_in,
                                        MPIDI_algo_parameters_t **nm_algo_parameters_ptr_out)
{
    int i;
    MPI_Aint type_size;
    int nbytes;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = &MPIDI_OFI_COMM(comm_ptr).colltuner_table[MPIDI_ALLREDUCE];

    if(ch4_algo_parameters_ptr_in->ch4_allreduce.nm_allreduce == -1 )
    {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size*count;

        for(i = 0; i < tuner_table_ptr->table_size; i++) 
        {
            if(tuner_table_ptr->table[i]->msg_size < nbytes)
            {
                continue;
            }
            else
            {
                *nm_algo_parameters_ptr_out = &(tuner_table_ptr->table[i]->params);
                return tuner_table_ptr->table[i]->algo_id;
            }
        }
    }
    else
    {
        *nm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_CH4_allreduce_ofi_param_defaults[ch4_algo_parameters_ptr_in->ch4_allreduce.nm_allreduce];
        return ch4_algo_parameters_ptr_in->ch4_allreduce.nm_allreduce;
    }

    //default parameters and algo_id
    *nm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_CH4_allreduce_ofi_param_defaults[tuner_table_ptr->table[0]->algo_id];
    return tuner_table_ptr->table[0]->algo_id;
}

static inline int MPIDI_NM_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Errflag_t * errflag,
                                          int algo_number, MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 9:
            mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        case 11:
            mpi_errno = MPIDI_NM_Allreduce_1(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag, ch4_algo_parameters_ptr); 
            break;
        default:
            mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;
    }

    return mpi_errno;
}

static inline int MPIDI_NM_Reduce_select(const void *sendbuf, void *recvbuf, int count,
                                        MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                        MPIR_Errflag_t * errflag, 
                                        MPIDI_algo_parameters_t *ch4_algo_parameters_ptr_in,
                                        MPIDI_algo_parameters_t **nm_algo_parameters_ptr_out)
{
    int i;
    MPI_Aint type_size;
    int nbytes;
    MPIDI_coll_tuner_table_t *tuner_table_ptr = &MPIDI_OFI_COMM(comm_ptr).colltuner_table[MPIDI_REDUCE];

    if(ch4_algo_parameters_ptr_in->ch4_reduce.nm_reduce == -1 )
    {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size*count;

        for(i = 0; i < tuner_table_ptr->table_size; i++) 
        {
            if(tuner_table_ptr->table[i]->msg_size < nbytes)
            {
                continue;
            }
            else
            {
                *nm_algo_parameters_ptr_out = &(tuner_table_ptr->table[i]->params);
                return tuner_table_ptr->table[i]->algo_id;
            }
        }
    }
    else
    {
        *nm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_CH4_reduce_ofi_param_defaults[ch4_algo_parameters_ptr_in->ch4_reduce.nm_reduce];
        return ch4_algo_parameters_ptr_in->ch4_reduce.nm_reduce;  
    }

    //default parameters and algo_id
    *nm_algo_parameters_ptr_out = (MPIDI_algo_parameters_t *)&MPIDI_CH4_reduce_ofi_param_defaults[tuner_table_ptr->table[0]->algo_id];
    return tuner_table_ptr->table[0]->algo_id;
}

static inline int MPIDI_NM_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag, int algo_number, 
                                       MPIDI_algo_parameters_t *ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch(algo_number)
    {
        case 15:
            mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        case 19:
            mpi_errno = MPIDI_NM_Reduce_1(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag, ch4_algo_parameters_ptr); 
            break;
        default:
            mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
            break;
    }

    return mpi_errno;
}

#endif /* NETMOD_OFI_COLL_SELECT_H_INCLUDED */