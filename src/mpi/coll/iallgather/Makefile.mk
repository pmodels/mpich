##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                              \
    src/mpi/coll/iallgather/iallgather.c

mpi_core_sources +=                                         \
    src/mpi/coll/iallgather/iallgather_intra_sched_recursive_doubling.c \
    src/mpi/coll/iallgather/iallgather_intra_sched_brucks.c             \
    src/mpi/coll/iallgather/iallgather_intra_sched_ring.c               \
    src/mpi/coll/iallgather/iallgather_inter_sched_local_gather_remote_bcast.c \
    src/mpi/coll/iallgather/iallgather_gentran_algos.c                    \
    src/mpi/coll/iallgather/iallgather_intra_gentran_recexch_doubling.c    \
    src/mpi/coll/iallgather/iallgather_intra_gentran_recexch_halving.c    \
    src/mpi/coll/iallgather/iallgather_intra_gentran_brucks.c				\
    src/mpi/coll/iallgather/iallgather_intra_gentran_ring.c
