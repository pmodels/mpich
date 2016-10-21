#ifndef NETMOD_OFI_COLL_SELECT_H_INCLUDED
#define NETMOD_OFI_COLL_SELECT_H_INCLUDED

#include "ofi_impl.h"
#include "ch4_coll_select.h"
#include "ch4_coll_params.h"
#include "coll_algo_params.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Bcast_knomial(void *buffer,
                                                     int count,
                                                     MPI_Datatype datatype,
                                                     int root,
                                                     MPIR_Comm * comm_ptr,
                                                     MPIR_Errflag_t * errflag,
                                                     MPIDI_OFI_coll_algo_container_t *
                                                     params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Allreduce_1(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                   MPIDI_OFI_coll_algo_container_t *
                                                   params_container) MPL_STATIC_INLINE_SUFFIX;
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Reduce_1(const void *sendbuf, void *recvbuf, int count,
                                                MPI_Datatype datatype, MPI_Op op, int root,
                                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                MPIDI_OFI_coll_algo_container_t *
                                                params_container) MPL_STATIC_INLINE_SUFFIX;

MPL_STATIC_INLINE_PREFIX void MPIDI_NM_collective_selection_init(MPIR_Comm * comm)
{
    int coll_id;
    MPIDI_coll_tuner_table_t *tuner_table_ptr;

    tuner_table_ptr =
        (MPIDI_coll_tuner_table_t *) MPL_malloc(MPIDI_NUM_COLLECTIVES *
                                                sizeof(MPIDI_coll_tuner_table_t));

    for (coll_id = 0; coll_id < MPIDI_NUM_COLLECTIVES; coll_id++) {
        tuner_table_ptr[coll_id].table_size = MPIDI_coll_table_size[NETMOD][coll_id];

        if (tuner_table_ptr[coll_id].table_size) {
            tuner_table_ptr[coll_id].table = MPIDI_coll_tuning_table[NETMOD][coll_id];
        }

        MPIDI_OFI_COMM(comm).colltuner_table = tuner_table_ptr;
    }
}


MPL_STATIC_INLINE_PREFIX void MPIDI_NM_collective_selection_free(MPIR_Comm * comm)
{
    MPIDI_coll_tuner_table_t *tuner_table_ptr;

    tuner_table_ptr = MPIDI_OFI_COMM(comm).colltuner_table;

    if (tuner_table_ptr)
        MPL_free(tuner_table_ptr);
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Bcast_select(void *buffer, int count,
                                                                                 MPI_Datatype datatype,
                                                                                 int root,
                                                                                 MPIR_Comm * comm_ptr,
                                                                                 MPIR_Errflag_t *
                                                                                 errflag,
                                                                                 MPIDI_OFI_coll_algo_container_t
                                                                                 *
                                                                                 ch4_algo_parameters_container_in)
{
    MPI_Aint type_size;
    int nbytes;
    int i = 0;

    if (ch4_algo_parameters_container_in->id != MPIDI_CH4_COLL_AUTO_SELECT) {
        return ch4_algo_parameters_container_in;
    }
    else {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size * count;

        MPIDI_coll_tuner_table_t *tuning_table_ptr =
            &MPIDI_OFI_COMM(comm_ptr).colltuner_table[MPIDI_BCAST];
        MPIDI_coll_tuning_table_entry_t *entry = tuning_table_ptr->table;

        while (nbytes > entry[i].msg_size && entry[i].msg_size != -1 && i < tuning_table_ptr->table_size) {
            i++;
        }

        return (MPIDI_OFI_coll_algo_container_t *)entry[i].algo_container;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Bcast_call(void *buffer, int count, MPI_Datatype datatype,
                                                  int root, MPIR_Comm * comm,
                                                  MPIR_Errflag_t * errflag,
                                                  MPIDI_OFI_coll_algo_container_t *
                                                  ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_bcast_knomial_id:
        mpi_errno =
            MPIDI_OFI_Bcast_knomial(buffer, count, datatype, root, comm, errflag,
                                    ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Allreduce_select(const void *sendbuf,
                                                                                     void *recvbuf,
                                                                                     int count,
                                                                                     MPI_Datatype
                                                                                     datatype, MPI_Op op,
                                                                                     MPIR_Comm *
                                                                                     comm_ptr,
                                                                                     MPIR_Errflag_t *
                                                                                     errflag,
                                                                                     MPIDI_OFI_coll_algo_container_t
                                                                                     *
                                                                                     ch4_algo_parameters_container_in)
{
    MPI_Aint type_size;
    int nbytes;
    int i = 0;

    if (ch4_algo_parameters_container_in->id != MPIDI_CH4_COLL_AUTO_SELECT) {
        return ch4_algo_parameters_container_in;
    }
    else {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size * count;

        MPIDI_coll_tuner_table_t *tuning_table_ptr =
            &MPIDI_OFI_COMM(comm_ptr).colltuner_table[MPIDI_ALLREDUCE];
        MPIDI_coll_tuning_table_entry_t *entry = tuning_table_ptr->table;

        while (nbytes > entry[i].msg_size && entry[i].msg_size != -1 && i < tuning_table_ptr->table_size) {
            i++;
        }

        return (MPIDI_OFI_coll_algo_container_t *)entry[i].algo_container;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Allreduce_call(const void *sendbuf, void *recvbuf, int count,
                                                      MPI_Datatype datatype, MPI_Op op,
                                                      MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                      MPIDI_OFI_coll_algo_container_t *
                                                      ch4_algo_parameters_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_ptr->id) {
    case MPIDI_OFI_allreduce_empty_id:
        mpi_errno =
            MPIDI_OFI_Allreduce_1(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag,
                                  ch4_algo_parameters_ptr);
        break;
    default:
        mpi_errno = MPIR_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX MPIDI_OFI_coll_algo_container_t *MPIDI_OFI_Reduce_select(const void *sendbuf,
                                                                                  void *recvbuf,
                                                                                  int count,
                                                                                  MPI_Datatype datatype,
                                                                                  MPI_Op op, int root,
                                                                                  MPIR_Comm * comm_ptr,
                                                                                  MPIR_Errflag_t *
                                                                                  errflag,
                                                                                  MPIDI_OFI_coll_algo_container_t
                                                                                  *
                                                                                  ch4_algo_parameters_container_in)
{
    MPI_Aint type_size;
    int nbytes;
    int i = 0;

    if (ch4_algo_parameters_container_in->id != MPIDI_CH4_COLL_AUTO_SELECT) {
        return ch4_algo_parameters_container_in;
    }
    else {
        MPID_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size * count;

        MPIDI_coll_tuner_table_t *tuning_table_ptr =
            &MPIDI_OFI_COMM(comm_ptr).colltuner_table[MPIDI_REDUCE];
        MPIDI_coll_tuning_table_entry_t *entry = tuning_table_ptr->table;

        while (nbytes > entry[i].msg_size && entry[i].msg_size != -1 && i < tuning_table_ptr->table_size) {
            i++;
        }

        return (MPIDI_OFI_coll_algo_container_t *)entry[i].algo_container;
    }
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_Reduce_call(const void *sendbuf, void *recvbuf, int count,
                                                   MPI_Datatype datatype, MPI_Op op, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag,
                                                   MPIDI_OFI_coll_algo_container_t *
                                                   ch4_algo_parameters_container)
{
    int mpi_errno = MPI_SUCCESS;

    switch (ch4_algo_parameters_container->id) {
    case MPIDI_OFI_reduce_empty_id:
        mpi_errno =
            MPIDI_OFI_Reduce_1(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag,
                               ch4_algo_parameters_container);
        break;
    default:
        mpi_errno = MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, errflag);
        break;
    }

    return mpi_errno;
}

#endif /* NETMOD_OFI_COLL_SELECT_H_INCLUDED */
