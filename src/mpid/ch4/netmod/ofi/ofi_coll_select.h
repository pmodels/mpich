#ifndef OFI_COLL_SELECT_H_INCLUDED
#define OFI_COLL_SELECT_H_INCLUDED

#include "ofi_impl.h"
#include "coll_algo_params.h"

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Barrier_select(MPIR_Comm * comm,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_OFI_coll_algo_container_t *
                                                               ch4_algo_parameters_container_in
                                                               ATTRIBUTE((unused)))
{
    return (MPIDI_OFI_coll_algo_container_t *) & OFI_Barrier_intra_dissemination_cnt;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Bcast_select(void *buffer, int count,
                                                             MPI_Datatype datatype,
                                                             int root,
                                                             MPIR_Comm * comm,
                                                             MPIR_Errflag_t * errflag,
                                                             MPIDI_OFI_coll_algo_container_t *
                                                             ch4_algo_parameters_container_in
                                                             ATTRIBUTE((unused)))
{
    int nbytes = 0;
    MPI_Aint type_size = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_Bcast_intra_binomial_cnt;
    } else {
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(comm->local_size, NULL)) {
            return (MPIDI_OFI_coll_algo_container_t *) &
                OFI_Bcast_intra_scatter_recursive_doubling_allgather_cnt;
        } else {
            return (MPIDI_OFI_coll_algo_container_t *) & OFI_Bcast_intra_scatter_ring_allgather_cnt;
        }
    }
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Allreduce_select(const void *sendbuf,
                                                                 void *recvbuf,
                                                                 int count,
                                                                 MPI_Datatype datatype, MPI_Op op,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag,
                                                                 MPIDI_OFI_coll_algo_container_t *
                                                                 ch4_algo_parameters_container_in
                                                                 ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int pof2 = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = comm->pof2;
    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_Allreduce_intra_recursive_doubling_cnt;
    } else {
        return (MPIDI_OFI_coll_algo_container_t *) &
            OFI_Allreduce_intra_reduce_scatter_allgather_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Reduce_select(const void *sendbuf,
                                                              void *recvbuf,
                                                              int count,
                                                              MPI_Datatype datatype,
                                                              MPI_Op op, int root,
                                                              MPIR_Comm * comm,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_OFI_coll_algo_container_t *
                                                              ch4_algo_parameters_container_in
                                                              ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int pof2 = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = comm->pof2;
    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_Reduce_intra_reduce_scatter_gather_cnt;
    } else {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_Reduce_intra_binomial_cnt;
    }
}

#endif /* OFI_COLL_SELECT_H_INCLUDED */
