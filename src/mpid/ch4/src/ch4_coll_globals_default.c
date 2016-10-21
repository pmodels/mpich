#include "coll_algo_params.h"
#include <mpidimpl.h>
#include "ch4_impl.h"

MPIDI_coll_algo_container_t mpir_fallback_container;

void * MPIDI_coll_get_next_container(void * container){

    return((void *) ((char *)container) + sizeof(MPIDI_COLL_ALGO_params_t));
}

const static MPIDI_coll_algo_container_t BCAST_TUNING_CH4_PARAMS[] =
{
   {
        .id = MPIDI_CH4_bcast_composition_alpha_id,
#ifdef MPIDI_BUILD_CH4_SHM
        .params.ch4_bcast_params.ch4_bcast.shm_bcast = MPIDI_POSIX_bcast_knomial_id,
#else
        .params.ch4_bcast_params.ch4_bcast.shm_bcast = MPIDI_CH4_COLL_AUTO_SELECT,
#endif

        .params.ch4_bcast_params.ch4_bcast.nm_bcast = MPIDI_OFI_bcast_knomial_id,
    },
    {
        .id = MPIDI_CH4_bcast_composition_shm_id,
        .params.ch4_bcast_params.ch4_bcast.shm_bcast = MPIDI_CH4_COLL_AUTO_SELECT,
        .params.ch4_bcast_params.ch4_bcast.nm_bcast = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    },
};

const MPIDI_coll_tuning_table_entry_t BCAST_TUNING_CH4[]=
{
    {
        .msg_size = 1024,
        .algo_container = &BCAST_TUNING_CH4_PARAMS[0],
    },
    {
        .msg_size = 2048,
        .algo_container = &BCAST_TUNING_CH4_PARAMS[1],
    },

};

const static MPIDI_OFI_coll_algo_container_t BCAST_TUNING_NETMOD_PARAMS[] =
{
    {
        .id = MPIDI_OFI_bcast_knomial_id,
        .params.ofi_bcast_params.ofi_bcast_knomial_parameters.radix = 2,
        .params.ofi_bcast_params.ofi_bcast_knomial_parameters.block_size = 4096,
    },
    {
        .id = MPIDI_OFI_bcast_empty_id,
        .params.ofi_bcast_params.ofi_bcast_empty_parameters = 1,
    },

};

const MPIDI_coll_tuning_table_entry_t BCAST_TUNING_NETMOD[]=
{
    {
        .msg_size = 2048,
        .algo_container = &BCAST_TUNING_NETMOD_PARAMS[0],
    },
    {
        .msg_size = 4096,
        .algo_container = &BCAST_TUNING_NETMOD_PARAMS[1],
    },

};

#ifdef MPIDI_BUILD_CH4_SHM
const static MPIDI_POSIX_coll_algo_container_t BCAST_TUNING_SHM_PARAMS[] =
{
    {
        .id = MPIDI_POSIX_bcast_knomial_id,
        .params.posix_bcast_params.posix_bcast_knomial_parameters.radix = 2,
        .params.posix_bcast_params.posix_bcast_knomial_parameters.block_size = 4096,
    },
    {
        .id = MPIDI_POSIX_bcast_empty_id,
        .params.posix_bcast_params.posix_bcast_empty_parameters = 1,
    },

};

const MPIDI_coll_tuning_table_entry_t BCAST_TUNING_SHM[]=
{
    {
        .msg_size = 2048,
        .algo_container = &BCAST_TUNING_SHM_PARAMS[0],
    },
    {
        .msg_size = 4096,
        .algo_container = &BCAST_TUNING_SHM_PARAMS[1],
    },

};
#endif /* MPIDI_BUILD_CH4_SHM */

const static MPIDI_coll_algo_container_t REDUCE_TUNING_CH4_PARAMS[] =
{
    {
        .id = MPIDI_CH4_reduce_composition_alpha_id,
        .params.ch4_reduce_params.ch4_reduce.shm_reduce = MPIDI_CH4_COLL_AUTO_SELECT,
        .params.ch4_reduce_params.ch4_reduce.nm_reduce = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    { 
        .id = MPIDI_CH4_reduce_composition_alpha_id,
        .params.ch4_reduce_params.ch4_reduce.shm_reduce = MPIDI_CH4_COLL_AUTO_SELECT,
        .params.ch4_reduce_params.ch4_reduce.nm_reduce = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    },
};

const MPIDI_coll_tuning_table_entry_t REDUCE_TUNING_CH4[]=
{
    {
        .msg_size = 0,
        .algo_container = &REDUCE_TUNING_CH4_PARAMS[0],
    },
    {
        .msg_size = 1024,
        .algo_container = &REDUCE_TUNING_CH4_PARAMS[3],
    },

};

const static MPIDI_OFI_coll_algo_container_t REDUCE_TUNING_NETMOD_PARAMS[] =
{
    {
        .id = MPIDI_OFI_reduce_empty_id,
        .params.ofi_reduce_params.ofi_reduce_empty_parameters.empty = 1,
    },
    {
        .id = MPIDI_OFI_reduce_empty_id,
        .params.ofi_reduce_params.ofi_reduce_empty_parameters.empty = 1,
    },
};

const MPIDI_coll_tuning_table_entry_t REDUCE_TUNING_NETMOD[]=
{
    {
        .msg_size = 0,
        .algo_container = &REDUCE_TUNING_NETMOD_PARAMS[0],
    },
    {
        .msg_size = 1024,
        .algo_container = &REDUCE_TUNING_NETMOD_PARAMS[1],
    },
};

#ifdef MPIDI_BUILD_CH4_SHM
const static MPIDI_POSIX_coll_algo_container_t REDUCE_TUNING_SHM_PARAMS[] =
{
    {
        .id = MPIDI_POSIX_reduce_empty_id,
        .params.posix_reduce_params.posix_reduce_empty_parameters.empty = 1,
    },
    {
        .id = MPIDI_POSIX_reduce_empty_id,
        .params.posix_reduce_params.posix_reduce_empty_parameters.empty = 1,
    },
};

const MPIDI_coll_tuning_table_entry_t REDUCE_TUNING_SHM[]=
{
    {
        .msg_size = 0,
        .algo_container = &REDUCE_TUNING_SHM_PARAMS[0],
    },
    {
        .msg_size = 1024,
        .algo_container = &REDUCE_TUNING_SHM_PARAMS[1],
    },
};
#endif /* MPIDI_BUILD_CH4_SHM */

const static MPIDI_coll_algo_container_t ALLREDUCE_TUNING_CH4_PARAMS[]=
{
    {
        .id = MPIDI_CH4_allreduce_composition_alpha_id,
        .params.ch4_allreduce_params.ch4_allreduce_alpha.shm_reduce = MPIDI_CH4_COLL_AUTO_SELECT,
        .params.ch4_allreduce_params.ch4_allreduce_alpha.nm_allreduce = MPIDI_CH4_COLL_AUTO_SELECT,
#ifdef MPIDI_BUILD_CH4_SHM
        .params.ch4_allreduce_params.ch4_allreduce_alpha.shm_bcast = MPIDI_POSIX_bcast_knomial_id,
#else
        .params.ch4_allreduce_params.ch4_allreduce_alpha.shm_bcast = MPIDI_CH4_COLL_AUTO_SELECT
#endif
    },
    {
        .id = MPIDI_CH4_allreduce_composition_alpha_id,
        .params.ch4_allreduce_params.ch4_allreduce_alpha.shm_reduce = MPIDI_CH4_COLL_AUTO_SELECT,
        .params.ch4_allreduce_params.ch4_allreduce_alpha.nm_allreduce = MPIDI_CH4_COLL_AUTO_SELECT,
        .params.ch4_allreduce_params.ch4_allreduce_alpha.shm_bcast = MPIDI_CH4_COLL_AUTO_SELECT,
    },
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    }, 
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    }, 
    {
        .id = MPIDI_CH4_COLL_AUTO_SELECT,
    }, 
};


const MPIDI_coll_tuning_table_entry_t ALLREDUCE_TUNING_CH4[]=
{
    {
        .msg_size = 0,
        .algo_container = &ALLREDUCE_TUNING_CH4_PARAMS[0],
    },
    {
        .msg_size = 1024,
        .algo_container = &ALLREDUCE_TUNING_CH4_PARAMS[1],
    },
};

const static MPIDI_OFI_coll_algo_container_t ALLREDUCE_TUNING_NETMOD_PARAMS[]=
{
    {
        .id = MPIDI_OFI_allreduce_empty_id,
        .params.ofi_allreduce_params.ofi_allreduce_empty_parameters.empty = 1,
    },
    {
        .id = MPIDI_OFI_allreduce_empty_id,
        .params.ofi_allreduce_params.ofi_allreduce_empty_parameters.empty = 1,
    },
};

const MPIDI_coll_tuning_table_entry_t ALLREDUCE_TUNING_NETMOD[]=
{
    {
        .msg_size = 0,
        .algo_container = &ALLREDUCE_TUNING_NETMOD_PARAMS[0],
    },
    {
        .msg_size = 1024,
        .algo_container = &ALLREDUCE_TUNING_NETMOD_PARAMS[1],
    },
};

#ifdef MPIDI_BUILD_CH4_SHM
const static MPIDI_POSIX_coll_algo_container_t ALLREDUCE_TUNING_SHM_PARAMS[]=
{
    {

        .id = MPIDI_POSIX_allreduce_empty_id,
        .params.posix_allreduce_params.posix_allreduce_empty_parameters.empty = 1,
    },
    {
        .id = MPIDI_POSIX_allreduce_empty_id,
        .params.posix_allreduce_params.posix_allreduce_empty_parameters.empty = 1,
    },

};

const MPIDI_coll_tuning_table_entry_t ALLREDUCE_TUNING_SHM[]=
{
    {
        .msg_size = 0,
        .algo_container = &ALLREDUCE_TUNING_SHM_PARAMS[0],
    },
    {
        .msg_size = 1024,
        .algo_container = &ALLREDUCE_TUNING_SHM_PARAMS[1],
    },
};
#endif /* MPIDI_BUILD_CH4_SHM */
