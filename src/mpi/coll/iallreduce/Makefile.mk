##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=                                         \
    src/mpi/coll/iallreduce/iallreduce_intra_sched_naive.c              \
    src/mpi/coll/iallreduce/iallreduce_intra_sched_reduce_scatter_allgather.c  \
    src/mpi/coll/iallreduce/iallreduce_intra_sched_recursive_doubling.c \
    src/mpi/coll/iallreduce/iallreduce_intra_sched_smp.c                \
    src/mpi/coll/iallreduce/iallreduce_inter_sched_remote_reduce_local_bcast.c \
    src/mpi/coll/iallreduce/iallreduce_tsp_recursive_exchange_common.c \
    src/mpi/coll/iallreduce/iallreduce_tsp_recexch.c \
    src/mpi/coll/iallreduce/iallreduce_tsp_recexch_reduce_scatter_recexch_allgatherv.c \
    src/mpi/coll/iallreduce/iallreduce_tsp_ring.c \
    src/mpi/coll/iallreduce/iallreduce_tsp_tree.c \
    src/mpi/coll/iallreduce/iallreduce_tsp_auto.c
