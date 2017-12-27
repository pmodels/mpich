#ifndef SHM_POSIX_COLL_SELECT_H_INCLUDED
#define SHM_POSIX_COLL_SELECT_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "coll_algo_params.h"
#include "posix_coll_impl.h"

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t *MPIDI_POSIX_Barrier_select(MPIR_Comm * comm_ptr,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_barrier_recursive_doubling_cnt;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t *MPIDI_POSIX_Bcast_select(void *buffer,
                                                            int count, MPI_Datatype
                                                            datatype, int root,
                                                            MPIR_Comm * comm_ptr,
                                                            MPIR_Errflag_t * errflag,
                                                            MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int nbytes = 0;
    MPI_Aint type_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_bcast_binomial_cnt;
    } else {
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL)) {
            return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_bcast_scatter_doubling_allgather_cnt;
        } else {
            return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_bcast_scatter_ring_allgather_cnt;
        }
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t *MPIDI_POSIX_Allreduce_select(const void *sendbuf,
                                                                void *recvbuf,
                                                                int count,
                                                                MPI_Datatype datatype,
                                                                MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag,
                                                                MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    MPI_Aint type_size;
    int pof2;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = MPIU_pof2(comm_ptr->local_size);
    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_allreduce_recursive_doubling_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_allreduce_reduce_scatter_allgather_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t *MPIDI_POSIX_Reduce_select(const void *sendbuf,
                                                             void *recvbuf, int count,
                                                             MPI_Datatype datatype,
                                                             MPI_Op op, int root,
                                                             MPIR_Comm * comm_ptr,
                                                             MPIR_Errflag_t * errflag,
                                                             MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int comm_size, type_size, pof2;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = MPIU_pof2(comm_size);
    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_reduce_redscat_gather_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_reduce_binomial_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Gather_select(const void *sendbuf,
                                                              int sendcount,
                                                              MPI_Datatype sendtype,
                                                              void *recvbuf,
                                                              int recvcount,
                                                              MPI_Datatype recvtype,
                                                              int root,
                                                              MPIR_Comm * comm,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_POSIX_coll_algo_container_t
                                                              * ch4_algo_parameters_container_in
                                                              ATTRIBUTE((unused)))
{
    int rank = -1;
    MPI_Aint nbytes = 0;
    MPI_Aint sendtype_size = 0;
    MPI_Aint recvtype_size = 0;

    rank = comm->rank;

    // nbytes for root and non-root ranks are equal
    if (rank == root) {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount;
    } else {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount;
    }

    if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
        return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_gather_intra_binomial_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_gather_intra_binomial_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Gatherv_select(const void *sendbuf,
                                                               int sendcount,
                                                               MPI_Datatype sendtype,
                                                               void *recvbuf,
                                                               const int *recvcounts,
                                                               const int *displs,
                                                               MPI_Datatype recvtype,
                                                               int root,
                                                               MPIR_Comm * comm,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_POSIX_coll_algo_container_t
                                                               * ch4_algo_parameters_container_in
                                                               ATTRIBUTE((unused)))
{
    int comm_size = 0;
    int min_procs = 0;

    comm_size = comm->local_size;
    min_procs = MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS;

    if (min_procs == -1) {
        min_procs = comm_size + 1;      /* Disable ssend */
    } else if (min_procs == 0) {        /* backwards compatibility, use default value */
        MPIR_CVAR_GET_DEFAULT_INT(MPIR_CVAR_GATHERV_INTER_SSEND_MIN_PROCS, &min_procs);
    }
    if (comm_size >= min_procs) {
        return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_gatherv_intra_linear_ssend_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_gatherv_intra_linear_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Scatter_select(const void *sendbuf,
                                                               int sendcount,
                                                               MPI_Datatype sendtype,
                                                               void *recvbuf,
                                                               int recvcount,
                                                               MPI_Datatype recvtype,
                                                               int root,
                                                               MPIR_Comm * comm,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_POSIX_coll_algo_container_t
                                                               * ch4_algo_parameters_container_in
                                                               ATTRIBUTE((unused)))
{
    return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_scatter_intra_binomial_cnt;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Scatterv_select(const void *sendbuf,
                                                                const int *sendcounts,
                                                                const int *displs,
                                                                MPI_Datatype sendtype,
                                                                void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype recvtype,
                                                                int root,
                                                                MPIR_Comm * comm,
                                                                MPIR_Errflag_t * errflag,
                                                                MPIDI_POSIX_coll_algo_container_t
                                                                * ch4_algo_parameters_container_in
                                                                ATTRIBUTE((unused)))
{
    return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_scatterv_intra_linear_cnt;
}

#endif /* SHM_POSIX_COLL_SELECT_H_INCLUDED */
