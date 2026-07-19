##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=                          \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_sched_recursive_halving.c  \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_sched_pairwise.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_sched_recursive_doubling.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_sched_noncommutative.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_tsp_recexch.c
