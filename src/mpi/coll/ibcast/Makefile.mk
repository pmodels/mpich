##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources += \
    src/mpi/coll/ibcast/ibcast_intra_sched_binomial.c                             \
    src/mpi/coll/ibcast/ibcast_intra_sched_scatter_ring_allgather.c               \
    src/mpi/coll/ibcast/ibcast_intra_sched_scatter_recursive_doubling_allgather.c \
    src/mpi/coll/ibcast/ibcast_intra_sched_smp.c                                  \
    src/mpi/coll/ibcast/ibcast_inter_sched_flat.c                                 \
    src/mpi/coll/ibcast/ibcast_tsp_scatterv_allgatherv.c \
    src/mpi/coll/ibcast/ibcast_tsp_scatterv_ring_allgatherv.c \
    src/mpi/coll/ibcast/ibcast_tsp_tree.c 					  \
    src/mpi/coll/ibcast/ibcast_tsp_auto.c 					  \
    src/mpi/coll/ibcast/ibcast_utils.c

noinst_HEADERS += \
    src/mpi/coll/ibcast/ibcast.h
