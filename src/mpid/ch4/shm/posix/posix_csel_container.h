/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_CSEL_CONTAINER_H_INCLUDED
#define POSIX_CSEL_CONTAINER_H_INCLUDED

typedef struct {
    enum {
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_bcast_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_impl,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_ibcast_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_sched_intra_gentran_auto,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_barrier_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_impl,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_allreduce_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_impl,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_reduce_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Reduce_impl,
    } id;
} MPIDI_POSIX_csel_container_s;

typedef struct {
    MPIDI_POSIX_csel_container_s id;

    union {
        struct {
            struct {
                int k;
                int tree_type;
                int buffer_size;
                int num_buffers;
            } release_gather;
        } ibcast;
    } u;
} MPIDI_POSIX_csel_container_coll;

#endif /* POSIX_CSEL_CONTAINER_H_INCLUDED */
