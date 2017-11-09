#ifndef NETMOD_OFI_COLL_SELECT_H_INCLUDED
#define NETMOD_OFI_COLL_SELECT_H_INCLUDED

#include "ofi_impl.h"
#include "coll_algo_params.h"
#include "ofi_coll_impl.h"

MPL_STATIC_INLINE_PREFIX
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Barrier_select(MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag,
                                                          MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    return (MPIDI_OFI_coll_algo_container_t *) &OFI_barrier_recursive_doubling_cnt;
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_OFI_Barrier_call(MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag,
                           MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_barrier_recursive_doubling_id:
        mpi_errno =
            MPIDI_OFI_Barrier_recursive_doubling(comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Barrier(comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Bcast_select(void *buffer, int count,
                                                        MPI_Datatype datatype,
                                                        int root,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag,
                                                        MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int nbytes = 0;
    MPI_Aint type_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) ||
        (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        return (MPIDI_OFI_coll_algo_container_t *) &OFI_bcast_binomial_cnt;
    } else {
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL)) {
            return (MPIDI_OFI_coll_algo_container_t *) &OFI_bcast_scatter_doubling_allgather_cnt;
        } else {
            return (MPIDI_OFI_coll_algo_container_t *) &OFI_bcast_scatter_ring_allgather_cnt;
        }
    }
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_OFI_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                         int root, MPIR_Comm * comm_ptr,
                         MPIR_Errflag_t * errflag,
                         MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_bcast_binomial_id:
        mpi_errno =
            MPIDI_OFI_Bcast_binomial(buffer, count, datatype, root, comm_ptr, errflag,
                                     ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_bcast_scatter_doubling_allgather_id:
        mpi_errno =
            MPIDI_OFI_Bcast_scatter_doubling_allgather(buffer, count, datatype, root, comm_ptr, errflag,
                                                       ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_bcast_scatter_ring_allgather_id:
        mpi_errno =
            MPIDI_OFI_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr, errflag,
                                                   ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Allreduce_select(const void *sendbuf,
                                                             void *recvbuf,
                                                             int count,
                                                             MPI_Datatype datatype, MPI_Op op,
                                                             MPIR_Comm * comm_ptr,
                                                             MPIR_Errflag_t * errflag,
                                                             MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    MPI_Aint type_size;
    int pof2;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = MPIU_pof2(comm_ptr->local_size);
    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
        return (MPIDI_OFI_coll_algo_container_t *) &OFI_allreduce_recursive_doubling_cnt;
    } else {
        return (MPIDI_OFI_coll_algo_container_t *) &OFI_allreduce_reduce_scatter_allgather_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_OFI_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                             MPI_Datatype datatype, MPI_Op op,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                             MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;
    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_allreduce_recursive_doubling_id:
        mpi_errno =
            MPIDI_OFI_allreduce_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                   errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_allreduce_reduce_scatter_allgather_id:
        mpi_errno =
            MPIDI_OFI_allreduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op,
                                                         comm_ptr, errflag,
                                                         ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Reduce_select(const void *sendbuf,
                                                          void *recvbuf,
                                                          int count,
                                                          MPI_Datatype datatype,
                                                          MPI_Op op, int root,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag,
                                                          MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int comm_size, type_size, pof2;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    pof2 = MPIU_pof2(comm_size);
    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        return (MPIDI_OFI_coll_algo_container_t *) &OFI_reduce_redscat_gather_cnt;
    } else {
        return (MPIDI_OFI_coll_algo_container_t *) &OFI_reduce_binomial_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
int MPIDI_OFI_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                          MPI_Datatype datatype, MPI_Op op, int root,
                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                          MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_reduce_redscat_gather_id:
        mpi_errno =
            MPIDI_OFI_reduce_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                            errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_reduce_binomial_id:
        mpi_errno =
            MPIDI_OFI_reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                      errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Alltoall_select(const void *sendbuf,
                                                                int sendcount,
                                                                MPI_Datatype sendtype,
                                                                void *recvbuf,
                                                                int recvcount,
                                                                MPI_Datatype recvtype,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag,
                                                                MPIDI_OFI_coll_algo_container_t *
                                                                ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    MPI_Aint type_size;
    int nbytes = 0;

    MPIR_Datatype_get_size_macro(sendtype, type_size);
    nbytes = sendcount * type_size;

    if (sendbuf == MPI_IN_PLACE) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoall_sendrecv_replace_cnt;
    }
    else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_ptr->local_size >= 8)) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoall_bruck_cnt;
    }
    else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoall_isend_irecv_cnt;
    }
    else {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoall_pairwise_exchange_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_Alltoall_call(const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag,
                                MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_alltoall_bruck_id:
        mpi_errno =
            MPIDI_OFI_alltoall_bruck(sendbuf, sendcount, sendtype, 
                                     recvbuf, recvcount, recvtype, 
                                     comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_alltoall_isend_irecv_id:
        mpi_errno =
            MPIDI_OFI_alltoall_isend_irecv(sendbuf, sendcount, sendtype, 
                                           recvbuf, recvcount, recvtype, 
                                           comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_alltoall_pairwise_exchange_id:
        mpi_errno =
            MPIDI_OFI_alltoall_pairwise_exchange(sendbuf, sendcount, sendtype, 
                                                 recvbuf, recvcount, recvtype, 
                                                 comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_alltoall_sendrecv_replace_id:
        mpi_errno =
            MPIDI_OFI_alltoall_sendrecv_replace(sendbuf, sendcount, sendtype, 
                                                recvbuf, recvcount, recvtype, 
                                                comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Alltoall(sendbuf, sendcount, sendtype, 
                                  recvbuf, recvcount, recvtype, 
                                  comm_ptr, errflag);
        break;
    }

    return mpi_errno;

}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Alltoallv_select(const void *sendbuf,
                                                                 const int *sendcounts,
                                                                 const int *sdispls,
                                                                 MPI_Datatype sendtype,
                                                                 void *recvbuf,
                                                                 const int *recvcounts,
                                                                 const int *rdispls,
                                                                 MPI_Datatype recvtype,
                                                                 MPIR_Comm *comm_ptr,
                                                                 MPIR_Errflag_t *errflag,
                                                                 MPIDI_OFI_coll_algo_container_t *
                                                                 ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    if (sendbuf == MPI_IN_PLACE) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoallv_sendrecv_replace_cnt;
    }
    else {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoallv_isend_irecv_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_Alltoallv_call(const void *sendbuf, const int *sendcounts, const int *sdispls,
                                 MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
                                 const int *rdispls, MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                                 MPIR_Errflag_t *errflag, MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_alltoallv_sendrecv_replace_id:
        mpi_errno =
            MPIDI_OFI_alltoallv_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                 sendtype, recvbuf, recvcounts,
                                                 rdispls, recvtype, comm_ptr,
                                                 errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_alltoallv_isend_irecv_id:
        mpi_errno =
            MPIDI_OFI_alltoallv_isend_irecv(sendbuf, sendcounts, sdispls,
                                            sendtype, recvbuf, recvcounts,
                                            rdispls, recvtype, comm_ptr,
                                            errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Alltoallv(sendbuf, sendcounts, sdispls,
                                   sendtype, recvbuf, recvcounts,
                                   rdispls, recvtype, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Alltoallw_select(const void *sendbuf,
                                                                 const int sendcounts[],
                                                                 const int sdispls[],
                                                                 const MPI_Datatype sendtypes[],
                                                                 void *recvbuf,
                                                                 const int recvcounts[],
                                                                 const int rdispls[],
                                                                 const MPI_Datatype recvtypes[],
                                                                 MPIR_Comm *comm_ptr,
                                                                 MPIR_Errflag_t *errflag,
                                                                 MPIDI_OFI_coll_algo_container_t *
                                                                 ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    if (sendbuf == MPI_IN_PLACE) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoallw_sendrecv_replace_cnt;
    }
    else {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_alltoallw_isend_irecv_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_Alltoallw_call(const void *sendbuf, const int sendcounts[], const int sdispls[],
                                 const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[],
                                 const int rdispls[], const MPI_Datatype recvtypes[],
                                 MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag,
                                 MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_alltoallw_sendrecv_replace_id:
        mpi_errno =
            MPIDI_OFI_alltoallw_sendrecv_replace(sendbuf, sendcounts, sdispls,
                                                 sendtypes, recvbuf, recvcounts,
                                                 rdispls, recvtypes, comm_ptr,
                                                 errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_alltoallw_isend_irecv_id:
        mpi_errno =
            MPIDI_OFI_alltoallw_isend_irecv(sendbuf, sendcounts, sdispls,
                                            sendtypes, recvbuf, recvcounts,
                                            rdispls, recvtypes, comm_ptr,
                                            errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_alltoallw_pairwise_exchange_id:
        mpi_errno =
            MPIDI_OFI_alltoallw_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                  sendtypes, recvbuf, recvcounts,
                                                  rdispls, recvtypes, comm_ptr,
                                                  errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Alltoallw(sendbuf, sendcounts, sdispls,
                                   sendtypes, recvbuf, recvcounts,
                                   rdispls, recvtypes, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Allgather_select(const void *sendbuf, int sendcount,
                                                                 MPI_Datatype sendtype, void *recvbuf,
                                                                 int recvcount, MPI_Datatype recvtype,
                                                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                                 MPIDI_OFI_coll_algo_container_t *
                                                                 ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int comm_size = 0;
    int type_size = 0;
    int nbytes = 0;
 
    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, type_size);
    nbytes = (MPI_Aint) recvcount * comm_size * type_size;

    if ((nbytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_allgather_recursive_doubling_cnt;
    }
    else if (nbytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_allgather_bruck_cnt;
    }
    else {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_allgather_ring_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_Allgather_call(const void *sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void *recvbuf,
                                 int recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                 MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_allgather_recursive_doubling_id:
        mpi_errno =
            MPIDI_OFI_allgather_recursive_doubling(sendbuf, sendcount, sendtype,
                                                   recvbuf, recvcount, recvtype,
                                                   comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_allgather_bruck_id:
        mpi_errno =
            MPIDI_OFI_allgather_bruck(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcount, recvtype,
                                      comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_allgather_ring_id:
        mpi_errno =
            MPIDI_OFI_allgather_ring(sendbuf, sendcount, sendtype,
                                     recvbuf, recvcount, recvtype,
                                     comm_ptr, errflag, ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Allgather(sendbuf, sendcount, sendtype,
                                   recvbuf, recvcount, recvtype,
                                   comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX
    MPIDI_OFI_coll_algo_container_t * MPIDI_OFI_Allgatherv_select(const void *sendbuf, int sendcount,
                                                                  MPI_Datatype sendtype, void *recvbuf,
                                                                  const int *recvcounts, const int *displs,
                                                                  MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                                                                  MPIR_Errflag_t *errflag,
                                                                  MPIDI_OFI_coll_algo_container_t *
                                                                  ch4_algo_parameters_container_in ATTRIBUTE((unused)))
{
    int comm_size = 0;
    int type_size = 0;
    int nbytes = 0;
    int i = 0;
    int total_count = 0;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, type_size);

    total_count = 0;
    for (i=0; i<comm_size; i++)
        total_count += recvcounts[i];
    
    nbytes = total_count * type_size;
    
    if ((nbytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) &&
        !(comm_size & (comm_size - 1))) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_allgatherv_recursive_doubling_cnt;
    }
    else if (nbytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_allgatherv_bruck_cnt;
    }
    else {
        return (MPIDI_OFI_coll_algo_container_t *) & OFI_allgatherv_ring_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_Allgatherv_call(const void *sendbuf, int sendcount,
                                  MPI_Datatype sendtype, void *recvbuf,
                                  const int *recvcounts, const int *displs,
                                  MPI_Datatype recvtype, MPIR_Comm *comm_ptr,
                                  MPIR_Errflag_t *errflag,
                                  MPIDI_OFI_coll_algo_container_t * ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_allgather_recursive_doubling_id:
        mpi_errno =
            MPIDI_OFI_allgatherv_recursive_doubling(sendbuf, sendcount, sendtype,
                                                    recvbuf, recvcounts, displs,
                                                    recvtype, comm_ptr, errflag,
                                                    ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_allgather_bruck_id:
        mpi_errno =
            MPIDI_OFI_allgatherv_bruck(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcounts, displs,
                                       recvtype, comm_ptr, errflag,
                                       ch4_algo_parameters_container);
        break;
    case MPIDI_OFI_allgather_ring_id:
        mpi_errno =
            MPIDI_OFI_allgatherv_ring(sendbuf, sendcount, sendtype,
                                      recvbuf, recvcounts, displs,
                                      recvtype, comm_ptr, errflag,
                                      ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                                    recvbuf, recvcounts, displs,
                                    recvtype, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}
#endif /* NETMOD_OFI_COLL_SELECT_H_INCLUDED */
