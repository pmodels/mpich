##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=                                           \
    src/mpi/coll/iallgatherv/iallgatherv_intra_sched_recursive_doubling.c \
    src/mpi/coll/iallgatherv/iallgatherv_intra_sched_brucks.c             \
    src/mpi/coll/iallgatherv/iallgatherv_intra_sched_ring.c               \
    src/mpi/coll/iallgatherv/iallgatherv_inter_sched_remote_gather_local_bcast.c \
    src/mpi/coll/iallgatherv/iallgatherv_tsp_brucks.c \
    src/mpi/coll/iallgatherv/iallgatherv_tsp_recexch.c \
    src/mpi/coll/iallgatherv/iallgatherv_tsp_ring.c \
    src/mpi/coll/iallgatherv/iallgatherv_utils.c

noinst_HEADERS += \
    src/mpi/coll/iallgatherv/iallgatherv.h
