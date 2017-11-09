#ifndef SHM_POSIX_COLL_SELECT_H_INCLUDED
#define SHM_POSIX_COLL_SELECT_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "coll_algo_params.h"
#include "posix_coll_impl.h"

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Barrier_select(MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag,
                                                               MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Barrier_intra_recursive_doubling_cnt;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Bcast_select(void *buffer,
                                                             int count, MPI_Datatype
                                                             datatype, int root,
                                                             MPIR_Comm * comm_ptr,
                                                             MPIR_Errflag_t * errflag,
                                                             MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int nbytes = 0;
    MPI_Aint type_size = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Bcast_intra_binomial_cnt;
    } else {
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL)) {
            return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Bcast_intra_scatter_doubling_allgather_cnt;
        } else {
            return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Bcast_intra_scatter_ring_allgather_cnt;
        }
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Allreduce_select(const void *sendbuf,
                                                                 void *recvbuf,
                                                                 int count,
                                                                 MPI_Datatype datatype,
                                                                 MPI_Op op,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Errflag_t * errflag,
                                                                 MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int pof2 = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = MPIU_pof2(comm_ptr->local_size);
    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allreduce_intra_recursive_doubling_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allreduce_intra_reduce_scatter_allgather_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Reduce_select(const void *sendbuf,
                                                              void *recvbuf, int count,
                                                              MPI_Datatype datatype,
                                                              MPI_Op op, int root,
                                                              MPIR_Comm * comm_ptr,
                                                              MPIR_Errflag_t * errflag,
                                                              MPIDI_POSIX_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int comm_size = 0;
    MPI_Aint type_size = 0;
    int pof2 = 0;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = MPIU_pof2(comm_size);
    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Reduce_intra_redscat_gather_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Reduce_intra_binomial_cnt;
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

    /* nbytes for root and non-root ranks are equal */
    if (rank == root) {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount;
    } else {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount;
    }

    if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Gather_intra_binomial_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Gather_intra_binomial_cnt;
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
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Gatherv_intra_linear_ssend_cnt;
    } else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Gatherv_intra_linear_cnt;
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
    return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Scatter_intra_binomial_cnt;
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
    return (MPIDI_POSIX_coll_algo_container_t *) & POSIX_Scatterv_intra_linear_cnt;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Alltoall_select(const void *sendbuf,
                                                                int sendcount,
                                                                MPI_Datatype sendtype,
                                                                void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype recvtype,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag,
                                                                MPIDI_POSIX_coll_algo_container_t
                                                                *
                                                                ch4_algo_parameters_container_in
                                                                ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int nbytes = 0;

    MPIR_Datatype_get_size_macro(sendtype, type_size);
    nbytes = sendcount * type_size;

    if (sendbuf == MPI_IN_PLACE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoall_intra_pairwise_sendrecv_replace_cnt;
    }
    else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_ptr->local_size >= 8)) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoall_intra_brucks_cnt;
    }
    else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoall_intra_scattered_cnt;
    }
    else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoall_intra_pairwise_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Alltoallv_select(const void *sendbuf,
                                                                 const int *sendcounts,
                                                                 const int *sdispls,
                                                                 MPI_Datatype sendtype,
                                                                 void *recvbuf,
                                                                 const int *recvcounts,
                                                                 const int *rdispls,
                                                                 MPI_Datatype recvtype,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Errflag_t * errflag,
                                                                 MPIDI_POSIX_coll_algo_container_t
                                                                 *
                                                                 ch4_algo_parameters_container_in
                                                                 ATTRIBUTE((unused)))
{
    if (sendbuf == MPI_IN_PLACE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoallv_intra_pairwise_sendrecv_replace_cnt;
    }
    else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoallv_intra_scattered_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Alltoallw_select(const void *sendbuf,
                                                                 const int sendcounts[],
                                                                 const int sdispls[],
                                                                 const MPI_Datatype sendtypes[],
                                                                 void *recvbuf,
                                                                 const int recvcounts[],
                                                                 const int rdispls[],
                                                                 const MPI_Datatype recvtypes[],
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Errflag_t * errflag,
                                                                 MPIDI_POSIX_coll_algo_container_t
                                                                 *
                                                                 ch4_algo_parameters_container_in
                                                                 ATTRIBUTE((unused)))
{
    if (sendbuf == MPI_IN_PLACE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoallw_intra_pairwise_sendrecv_replace_cnt;
    }
    else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Alltoallw_intra_scattered_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Allgather_select(const void *sendbuf,
                                                                 int sendcount,
                                                                 MPI_Datatype sendtype,
                                                                 void *recvbuf, int recvcount,
                                                                 MPI_Datatype recvtype,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Errflag_t * errflag,
                                                                 MPIDI_POSIX_coll_algo_container_t
                                                                 *
                                                                 ch4_algo_parameters_container_in)
{
    int comm_size = 0;
    MPI_Aint type_size = 0;
    int nbytes = 0;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, type_size);
    nbytes = (MPI_Aint) recvcount *comm_size * type_size;

    if ((nbytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allgather_intra_recursive_doubling_cnt;
    }
    else if (nbytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allgather_intra_brucks_cnt;
    }
    else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allgather_intra_ring_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Allgatherv_select(const void *sendbuf,
                                                                  int sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf,
                                                                  const int *recvcounts,
                                                                  const int *displs,
                                                                  MPI_Datatype recvtype,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag,
                                                                  MPIDI_POSIX_coll_algo_container_t
                                                                  *
                                                                  ch4_algo_parameters_container_in)
{
    int comm_size = 0;
    MPI_Aint type_size = 0;
    int nbytes = 0;
    int i = 0;
    int total_count = 0;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, type_size);

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    nbytes = total_count * type_size;

    if ((nbytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allgatherv_intra_recursive_doubling_cnt;
    }
    else if (nbytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allgatherv_intra_brucks_cnt;
    }
    else {
        return (MPIDI_POSIX_coll_algo_container_t *) &POSIX_Allgatherv_intra_ring_cnt;
    }
}
#endif /* SHM_POSIX_COLL_SELECT_H_INCLUDED */
