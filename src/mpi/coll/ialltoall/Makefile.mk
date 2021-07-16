##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=                             \
    src/mpi/coll/ialltoall/ialltoall_intra_sched_inplace.c  \
    src/mpi/coll/ialltoall/ialltoall_intra_sched_brucks.c   \
    src/mpi/coll/ialltoall/ialltoall_intra_sched_permuted_sendrecv.c  \
    src/mpi/coll/ialltoall/ialltoall_intra_sched_pairwise.c \
    src/mpi/coll/ialltoall/ialltoall_inter_sched_pairwise_exchange.c \
    src/mpi/coll/ialltoall/ialltoall_tsp_brucks.c \
    src/mpi/coll/ialltoall/ialltoall_tsp_ring.c \
    src/mpi/coll/ialltoall/ialltoall_tsp_scattered.c
