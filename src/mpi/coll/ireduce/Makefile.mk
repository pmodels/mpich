##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources += \
    src/mpi/coll/ireduce/ireduce_intra_sched_binomial.c                   \
    src/mpi/coll/ireduce/ireduce_intra_sched_reduce_scatter_gather.c      \
    src/mpi/coll/ireduce/ireduce_intra_sched_smp.c                        \
    src/mpi/coll/ireduce/ireduce_inter_sched_local_reduce_remote_send.c	\
    src/mpi/coll/ireduce/ireduce_tsp_tree.c \
    src/mpi/coll/ireduce/ireduce_tsp_auto.c
