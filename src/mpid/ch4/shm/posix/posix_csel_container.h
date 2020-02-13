#ifndef POSIX_CSEL_CONTAINER_H_INCLUDED
#define POSIX_CSEL_CONTAINER_H_INCLUDED

typedef struct {
    enum {
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_bcast_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_barrier_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_allreduce_release_gather,
        MPIDI_POSIX_CSEL_CONTAINER_TYPE__ALGORITHM__MPIDI_POSIX_mpi_reduce_release_gather,
    } id;
} MPIDI_POSIX_csel_container_s;

#endif /* POSIX_CSEL_CONTAINER_H_INCLUDED */
