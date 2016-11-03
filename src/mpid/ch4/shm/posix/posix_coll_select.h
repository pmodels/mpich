#ifndef SHM_POSIX_COLL_SELECT_H_INCLUDED
#define SHM_POSIX_COLL_SELECT_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"

static inline int MPIDI_SHM_collective_selection_init(MPIR_Comm * comm)
{
    int i, coll_id;
    coll_params_t *coll_params;

    coll_params = (coll_params_t *)MPL_malloc(COLLECTIVE_NUMBER*sizeof(coll_params_t));

    for( coll_id = 0; coll_id < COLLECTIVE_NUMBER; coll_id++ )
    {
        coll_params[coll_id].table_size = table_size[POSIX][coll_id];
        coll_params[coll_id].table      = (table_entry_t **)MPL_malloc(coll_params[coll_id].table_size*sizeof(table_entry_t*));
        if(coll_params[coll_id].table)
        {
            for( i = 0; i <  coll_params[coll_id].table_size; i++)
            {
                coll_params[coll_id].table[i] = &tuning_table[POSIX][coll_id][i];
            }
        }
        (coll_params_t *)MPIDI_POSIX_COMM(comm).coll_params = coll_params;
    }
}

static inline int MPIDI_SHM_collective_selection_free(MPIR_Comm * comm)
{

    int i, coll_id;
    coll_params_t* coll_params;

    coll_params = (coll_params_t *)MPIDI_POSIX_COMM(comm).coll_params;

    for( i = 0; i < COLLECTIVE_NUMBER; i++ )
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
                                         int root, coll_params_t * coll_params, MPIR_Errflag_t * errflag,
                                         algo_parameters_t **algo_parameters_ptr)
{
    //find pointer to table_entry, fill algo_parameters_ptr, return algorithm id
    int i;
    MPI_Aint type_size;
    int nbytes;

    MPID_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size*count;

    for(i = 0; i < coll_params->table_size; i++)
    {
        if(coll_params->table[i]->threshold < nbytes)
        {
            *algo_parameters_ptr = &(coll_params->table[i]->params);
            return coll_params->table[i]->algo_id;
        }
    }
    return 0;
}
#endif /* SHM_POSIX_COLL_SELECT_H_INCLUDED */