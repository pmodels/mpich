#ifndef SHM_POSIX_COLL_SELECT_H_INCLUDED
#define SHM_POSIX_COLL_SELECT_H_INCLUDED

#include "posix_impl.h"
#include "ch4_impl.h"
#include "coll_algo_params.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Barrier_recursive_doubling(MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag,
                                                                    MPIDI_POSIX_coll_algo_container_t *
                                                                    params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Bcast_binomial(void *buffer,
                                                        int count,
                                                        MPI_Datatype datatype,
                                                        int root,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag,
                                                        MPIDI_POSIX_coll_algo_container_t *
                                                        params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Bcast_scatter_doubling_allgather(void *buffer,
                                                                          int count,
                                                                          MPI_Datatype datatype,
                                                                          int root,
                                                                          MPIR_Comm * comm_ptr,
                                                                          MPIR_Errflag_t * errflag,
                                                                          MPIDI_POSIX_coll_algo_container_t *
                                                                          params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Bcast_scatter_ring_allgather(void *buffer,
                                                                      int count,
                                                                      MPI_Datatype datatype,
                                                                      int root,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t * errflag,
                                                                      MPIDI_POSIX_coll_algo_container_t *
                                                                      params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_allreduce_recursive_doubling(const void *sendbuf, void *recvbuf, int count,
                                                                      MPI_Datatype datatype, MPI_Op op,
                                                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                                      MPIDI_POSIX_coll_algo_container_t *
                                                                      params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_allreduce_reduce_scatter_allgather(const void *sendbuf, void *recvbuf, int count,
                                                                            MPI_Datatype datatype, MPI_Op op,
                                                                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                                            MPIDI_POSIX_coll_algo_container_t *
                                                                            params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Reduce_redscat_gather(const void *sendbuf, void *recvbuf, int count,
                                                               MPI_Datatype datatype, MPI_Op op, int root,
                                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                               MPIDI_POSIX_coll_algo_container_t *
                                                               params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Reduce_binomial(const void *sendbuf, void *recvbuf, int count,
                                                         MPI_Datatype datatype, MPI_Op op, int root,
                                                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                         MPIDI_POSIX_coll_algo_container_t *
                                                         params_container) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Barrier_select(MPIR_Comm * comm_ptr,
                                                                                        MPIR_Errflag_t *
                                                                                        errflag,
                                                                                        MPIDI_POSIX_coll_algo_container_t
                                                                                        *
                                                                                        ch4_algo_parameters_container_in)
{

    (void)ch4_algo_parameters_container_in;

    return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_barrier_recursive_doubling_cnt;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Barrier_call(MPIR_Comm * comm,
                                                      MPIR_Errflag_t * errflag,
                                                      MPIDI_POSIX_coll_algo_container_t *
                                                      ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_POSIX_barrier_recursive_doubling_id:
        mpi_errno =
            MPIDI_POSIX_Barrier_recursive_doubling(comm, errflag,
                                                   ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Barrier(comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Bcast_select(void *buffer,
                                                                                      int count,
                                                                                      MPI_Datatype
                                                                                      datatype,
                                                                                      int root,
                                                                                      MPIR_Comm *
                                                                                      comm_ptr,
                                                                                      MPIR_Errflag_t
                                                                                      * errflag,
                                                                                      MPIDI_POSIX_coll_algo_container_t
                                                                                      *
                                                                                      ch4_algo_parameters_container_in)
{
    int nbytes=0;
    int is_homogeneous;
    MPI_Aint type_size;

    (void)ch4_algo_parameters_container_in;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif
    if (is_homogeneous)
        MPID_Datatype_get_size_macro(datatype, type_size);
    else
        MPIR_Pack_size_impl(1, datatype, &type_size);

    nbytes = type_size * count;

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_ptr->local_size < MPIR_CVAR_BCAST_MIN_PROCS)){
        return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_bcast_binomial_cnt;
    }
    else{
        if (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPIU_is_pof2(comm_ptr->local_size, NULL)){
            return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_bcast_scatter_doubling_allgather_cnt;
       }
        else{
            return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_bcast_scatter_ring_allgather_cnt;
        }
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                                    int root, MPIR_Comm * comm,
                                                    MPIR_Errflag_t * errflag,
                                                    MPIDI_POSIX_coll_algo_container_t *
                                                    ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_POSIX_bcast_binomial_id:
        mpi_errno =
            MPIDI_POSIX_Bcast_binomial(buffer, count, datatype, root, comm, errflag,
                                       ch4_algo_parameters_container);
        break;
    case MPIDI_POSIX_bcast_scatter_doubling_allgather_id:
        mpi_errno =
            MPIDI_POSIX_Bcast_scatter_doubling_allgather(buffer, count, datatype, root, comm, errflag,
                                                         ch4_algo_parameters_container);
        break;
    case MPIDI_POSIX_bcast_scatter_ring_allgather_id:
        mpi_errno =
            MPIDI_POSIX_Bcast_scatter_ring_allgather(buffer, count, datatype, root, comm, errflag,
                                                     ch4_algo_parameters_container);
        break;                        
    default:
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Allreduce_select(const void
                                                                                          *sendbuf,
                                                                                          void
                                                                                          *recvbuf,
                                                                                          int count,
                                                                                          MPI_Datatype
                                                                                          datatype,
                                                                                          MPI_Op op,
                                                                                          MPIR_Comm *
                                                                                          comm_ptr,
                                                                                          MPIR_Errflag_t
                                                                                          * errflag,
                                                                                          MPIDI_POSIX_coll_algo_container_t
                                                                                          *
                                                                                          ch4_algo_parameters_container_in)
{
    MPI_Aint type_size;
    int nbytes;
    int pof2;

    (void)ch4_algo_parameters_container_in;

    MPID_Datatype_get_size_macro(datatype, type_size);
    pof2 = 1;
    while (pof2 <= comm_ptr->local_size) pof2 <<= 1;
    pof2 >>= 1;
    if ((count*type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) ||  
        (count < pof2)) {
        return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_allreduce_recursive_doubling_cnt;
    }
    else{
        return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_allreduce_reduce_scatter_allgather_cnt;
    }
}


MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Allreduce_call(const void *sendbuf, void *recvbuf,
                                                        int count, MPI_Datatype datatype, MPI_Op op,
                                                        MPIR_Comm * comm_ptr,
                                                        MPIR_Errflag_t * errflag,
                                                        MPIDI_POSIX_coll_algo_container_t *
                                                        ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_POSIX_allreduce_recursive_doubling_id:
        mpi_errno =
            MPIDI_POSIX_allreduce_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag,
                                                     ch4_algo_parameters_container);
        break;
    case MPIDI_POSIX_allreduce_reduce_scatter_allgather_id:
        mpi_errno =
            MPIDI_POSIX_allreduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag,
                                                           ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}


MPL_STATIC_INLINE_PREFIX MPIDI_POSIX_coll_algo_container_t * MPIDI_POSIX_Reduce_select(const void
                                                                                       *sendbuf,
                                                                                       void *recvbuf,
                                                                                       int count,
                                                                                       MPI_Datatype
                                                                                       datatype,
                                                                                       MPI_Op op,
                                                                                       int root,
                                                                                       MPIR_Comm *
                                                                                       comm_ptr,
                                                                                       MPIR_Errflag_t
                                                                                       * errflag,
                                                                                       MPIDI_POSIX_coll_algo_container_t
                                                                                       *
                                                                                       ch4_algo_parameters_container_in)
{
    int comm_size, type_size, pof2;

    (void)ch4_algo_parameters_container_in;

    comm_size = comm_ptr->local_size;
    MPID_Datatype_get_size_macro(datatype, type_size);
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>= 1;
    if ((count*type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) && (count >= pof2)) {
        return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_reduce_redscat_gather_cnt;
    }
    else{
        return (MPIDI_POSIX_coll_algo_container_t *)&POSIX_reduce_binomial_cnt;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                                     MPI_Datatype datatype, MPI_Op op, int root,
                                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                     MPIDI_POSIX_coll_algo_container_t *
                                                     ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_POSIX_reduce_redscat_gather_id:
        mpi_errno =
            MPIDI_POSIX_reduce_redscat_gather(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag,
                                              ch4_algo_parameters_container);
        break;
    case MPIDI_POSIX_reduce_binomial_id:
        mpi_errno =
            MPIDI_POSIX_reduce_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag,
                                        ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}
#endif /* SHM_POSIX_COLL_SELECT_H_INCLUDED */
