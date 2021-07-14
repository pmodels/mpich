##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=                               \
    src/mpi/coll/ialltoallv/ialltoallv_intra_sched_inplace.c  \
    src/mpi/coll/ialltoallv/ialltoallv_intra_sched_blocked.c  \
    src/mpi/coll/ialltoallv/ialltoallv_inter_sched_pairwise_exchange.c  \
    src/mpi/coll/ialltoallv/ialltoallv_tsp_blocked.c \
    src/mpi/coll/ialltoallv/ialltoallv_tsp_inplace.c \
    src/mpi/coll/ialltoallv/ialltoallv_tsp_scattered.c
