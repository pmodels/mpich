#ifndef OFI_COLL_SELECT_H_INCLUDED
#define OFI_COLL_SELECT_H_INCLUDED

#include "ofi_impl.h"
#include "coll_algo_params.h"

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Barrier_select(MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag, const void
                                                          *ch4_algo_parameters_container_in
                                                          ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Barrier_intra_dissemination_cnt;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Bcast_select(void *buffer, int count,
                                                        MPI_Datatype datatype,
                                                        int root,
                                                        MPIR_Comm * comm,
                                                        MPIR_Errflag_t * errflag,
                                                        const void *ch4_algo_parameters_container_in
                                                        ATTRIBUTE((unused)))
{
    int nbytes = 0;
    MPI_Aint type_size = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        return &MPIDI_OFI_Bcast_intra_binomial_cnt;
    } else {
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(comm->local_size, NULL)) {
            return &MPIDI_OFI_Bcast_intra_scatter_recursive_doubling_allgather_cnt;
        } else {
            return &MPIDI_OFI_Bcast_intra_scatter_ring_allgather_cnt;
        }
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Allreduce_select(const void *sendbuf,
                                                            void *recvbuf,
                                                            int count,
                                                            MPI_Datatype datatype, MPI_Op op,
                                                            MPIR_Comm * comm,
                                                            MPIR_Errflag_t * errflag, const void
                                                            *ch4_algo_parameters_container_in
                                                            ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int pof2 = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = comm->pof2;
    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        return &MPIDI_OFI_Allreduce_intra_recursive_doubling_cnt;
    } else {
        return &MPIDI_OFI_Allreduce_intra_reduce_scatter_allgather_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Reduce_select(const void *sendbuf,
                                                         void *recvbuf,
                                                         int count,
                                                         MPI_Datatype datatype,
                                                         MPI_Op op, int root,
                                                         MPIR_Comm * comm,
                                                         MPIR_Errflag_t * errflag, const void
                                                         *ch4_algo_parameters_container_in
                                                         ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int pof2 = 0;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = comm->pof2;
    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        return &MPIDI_OFI_Reduce_intra_reduce_scatter_gather_cnt;
    } else {
        return &MPIDI_OFI_Reduce_intra_binomial_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Gather_select(const void *sendbuf,
                                                         int sendcount,
                                                         MPI_Datatype sendtype,
                                                         void *recvbuf,
                                                         int recvcount,
                                                         MPI_Datatype recvtype,
                                                         int root,
                                                         MPIR_Comm * comm,
                                                         MPIR_Errflag_t * errflag, const void
                                                         *ch4_algo_parameters_container_in
                                                         ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Gather_intra_binomial_cnt;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Gatherv_select(const void *sendbuf,
                                                          int sendcount,
                                                          MPI_Datatype sendtype,
                                                          void *recvbuf,
                                                          const int *recvcounts,
                                                          const int *displs,
                                                          MPI_Datatype recvtype,
                                                          int root,
                                                          MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag, const void
                                                          *ch4_algo_parameters_container_in
                                                          ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Gatherv_allcomm_linear_cnt;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Scatter_select(const void *sendbuf,
                                                          int sendcount,
                                                          MPI_Datatype sendtype,
                                                          void *recvbuf,
                                                          int recvcount,
                                                          MPI_Datatype recvtype,
                                                          int root,
                                                          MPIR_Comm * comm,
                                                          MPIR_Errflag_t * errflag, const void
                                                          *ch4_algo_parameters_container_in
                                                          ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Scatter_intra_binomial_cnt;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Scatterv_select(const void *sendbuf,
                                                           const int *sendcounts,
                                                           const int *displs,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf,
                                                           int recvcount,
                                                           MPI_Datatype recvtype,
                                                           int root,
                                                           MPIR_Comm * comm,
                                                           MPIR_Errflag_t * errflag, const void
                                                           *ch4_algo_parameters_container_in
                                                           ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Scatterv_allcomm_linear_cnt;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Alltoall_select(const void *sendbuf,
                                                           int sendcount,
                                                           MPI_Datatype sendtype,
                                                           void *recvbuf,
                                                           int recvcount,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm_ptr,
                                                           MPIR_Errflag_t * errflag, const void
                                                           *ch4_algo_parameters_container_in
                                                           ATTRIBUTE((unused)))
{
    MPI_Aint type_size = 0;
    int nbytes = 0;

    MPIR_Datatype_get_size_macro(sendtype, type_size);
    nbytes = sendcount * type_size;

    if (sendbuf == MPI_IN_PLACE) {
        return &MPIDI_OFI_Alltoall_intra_pairwise_sendrecv_replace_cnt;
    } else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_ptr->local_size >= 8)) {
        return &MPIDI_OFI_Alltoall_intra_brucks_cnt;
    } else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        return &MPIDI_OFI_Alltoall_intra_scattered_cnt;
    } else {
        return &MPIDI_OFI_Alltoall_intra_pairwise_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Alltoallv_select(const void *sendbuf,
                                                            const int *sendcounts,
                                                            const int *sdispls,
                                                            MPI_Datatype sendtype,
                                                            void *recvbuf,
                                                            const int *recvcounts,
                                                            const int *rdispls,
                                                            MPI_Datatype recvtype,
                                                            MPIR_Comm * comm_ptr,
                                                            MPIR_Errflag_t * errflag, const void
                                                            *ch4_algo_parameters_container_in
                                                            ATTRIBUTE((unused)))
{
    if (sendbuf == MPI_IN_PLACE) {
        return &MPIDI_OFI_Alltoallv_intra_pairwise_sendrecv_replace_cnt;
    } else {
        return &MPIDI_OFI_Alltoallv_intra_scattered_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Alltoallw_select(const void *sendbuf,
                                                            const int sendcounts[],
                                                            const int sdispls[],
                                                            const MPI_Datatype sendtypes[],
                                                            void *recvbuf,
                                                            const int recvcounts[],
                                                            const int rdispls[],
                                                            const MPI_Datatype recvtypes[],
                                                            MPIR_Comm * comm_ptr,
                                                            MPIR_Errflag_t * errflag, const void
                                                            *ch4_algo_parameters_container_in
                                                            ATTRIBUTE((unused)))
{
    if (sendbuf == MPI_IN_PLACE) {
        return &MPIDI_OFI_Alltoallw_intra_pairwise_sendrecv_replace_cnt;
    } else {
        return &MPIDI_OFI_Alltoallw_intra_scattered_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Allgather_select(const void *sendbuf, int sendcount,
                                                            MPI_Datatype sendtype,
                                                            void *recvbuf, int recvcount,
                                                            MPI_Datatype recvtype,
                                                            MPIR_Comm * comm_ptr,
                                                            MPIR_Errflag_t * errflag, const void
                                                            *ch4_algo_parameters_container_in
                                                            ATTRIBUTE((unused)))
{
    int comm_size = 0;
    MPI_Aint type_size = 0;
    int nbytes = 0;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, type_size);
    nbytes = (MPI_Aint) recvcount *comm_size * type_size;

    if ((nbytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        return &MPIDI_OFI_Allgather_intra_recursive_doubling_cnt;
    } else if (nbytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        return &MPIDI_OFI_Allgather_intra_brucks_cnt;
    } else {
        return &MPIDI_OFI_Allgather_intra_ring_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Allgatherv_select(const void *sendbuf,
                                                             int sendcount,
                                                             MPI_Datatype sendtype,
                                                             void *recvbuf,
                                                             const int *recvcounts,
                                                             const int *displs,
                                                             MPI_Datatype recvtype,
                                                             MPIR_Comm * comm_ptr,
                                                             MPIR_Errflag_t * errflag, const void
                                                             *ch4_algo_parameters_container_in
                                                             ATTRIBUTE((unused)))
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
        return &MPIDI_OFI_Allgatherv_intra_recursive_doubling_cnt;
    } else if (nbytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        return &MPIDI_OFI_Allgatherv_intra_brucks_cnt;
    } else {
        return &MPIDI_OFI_Allgatherv_intra_ring_cnt;
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Reduce_scatter_select(const void *sendbuf,
                                                                 void *recvbuf,
                                                                 const int recvcounts[],
                                                                 MPI_Datatype datatype,
                                                                 MPI_Op op, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag,
                                                                 const void
                                                                 *ch4_algo_parameters_container_in
                                                                 ATTRIBUTE((unused)))
{
    int comm_size = 0;
    int i = 0;
    int type_size = 0;
    int total_count = 0;
    int nbytes = 0;
    int pof2 = 0;
    int is_commutative = 0;
    MPIR_Op *op_ptr = NULL;

    comm_size = comm->local_size;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
    } else {
        MPIR_Op_get_ptr(op, op_ptr);
        MPIR_Assert(op_ptr != NULL);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE) {
            is_commutative = 0;
        } else {
            is_commutative = 1;
        }
    }

    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        total_count += recvcounts[i];
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    if ((is_commutative) && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and short. use recursive halving algorithm */
        return &MPIDI_OFI_Reduce_scatter_intra_recursive_halving_cnt;
    }
    if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and long message, or noncommutative and long message.
         * use (p-1) pairwise exchanges */
        return &MPIDI_OFI_Reduce_scatter_intra_pairwise_cnt;
    }
    if (!is_commutative) {
        int is_block_regular = 1;
        for (i = 0; i < (comm_size - 1); ++i) {
            if (recvcounts[i] != recvcounts[i + 1]) {
                is_block_regular = 0;
                break;
            }
        }
        /* slightly retask pof2 to mean pof2 equal or greater, not always greater as it is above */
        pof2 = MPL_pof2(comm_size);
        if (pof2 == comm_size && is_block_regular) {
            /* noncommutative, pof2 size, and block regular */
            return &MPIDI_OFI_Reduce_scatter_intra_noncommutative_cnt;
        } else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */
            return &MPIDI_OFI_Reduce_scatter_intra_recursive_doubling_cnt;
        }
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Reduce_scatter_block_select(const void *sendbuf,
                                                                       void *recvbuf,
                                                                       int recvcount,
                                                                       MPI_Datatype datatype,
                                                                       MPI_Op op,
                                                                       MPIR_Comm * comm,
                                                                       MPIR_Errflag_t *
                                                                       errflag, const void
                                                                       *ch4_algo_parameters_container_in
                                                                       ATTRIBUTE((unused)))
{
    int comm_size = 0;
    int type_size = 0;
    int total_count = 0;
    int nbytes = 0;
    int is_commutative = 0;
    MPIR_Op *op_ptr = NULL;

    comm_size = comm->local_size;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
    } else {
        MPIR_Op_get_ptr(op, op_ptr);
        MPIR_Assert(op_ptr != NULL);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE) {
            is_commutative = 0;
        } else {
            is_commutative = 1;
        }
    }

    total_count = comm_size * recvcount;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    if ((is_commutative) && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and short. use recursive halving algorithm */
        return &MPIDI_OFI_Reduce_scatter_block_intra_recursive_halving_cnt;
    }
    if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        /* commutative and long message, or noncommutative and long message.
         * use (p-1) pairwise exchanges */
        return &MPIDI_OFI_Reduce_scatter_block_intra_pairwise_cnt;
    }
    if (!is_commutative) {
        /* power of two check */
        if (!(comm_size & (comm_size - 1))) {
            /* noncommutative, pof2 size */
            return &MPIDI_OFI_Reduce_scatter_block_intra_noncommutative_cnt;
        } else {
            /* noncommutative and non-pof2, use recursive doubling. */
            return &MPIDI_OFI_Reduce_scatter_block_intra_recursive_doubling_cnt;
        }
    }

    return NULL;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Scan_select(const void *sendbuf,
                                                       void *recvbuf,
                                                       int count,
                                                       MPI_Datatype datatype,
                                                       MPI_Op op,
                                                       MPIR_Comm * comm,
                                                       MPIR_Errflag_t *
                                                       errflag,
                                                       const void *ch4_algo_parameters_container_in
                                                       ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Scan_intra_recursive_doubling_cnt;
}

MPL_STATIC_INLINE_PREFIX const
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Exscan_select(const void *sendbuf,
                                                         void *recvbuf,
                                                         int count,
                                                         MPI_Datatype datatype,
                                                         MPI_Op op,
                                                         MPIR_Comm * comm,
                                                         MPIR_Errflag_t * errflag, const void
                                                         *ch4_algo_parameters_container_in
                                                         ATTRIBUTE((unused)))
{
    return &MPIDI_OFI_Exscan_intra_recursive_doubling_cnt;
}

#endif /* OFI_COLL_SELECT_H_INCLUDED */
