##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources

mpi_core_sources +=											\
    src/mpi/coll/bcast/bcast_utils.c						\
    src/mpi/coll/bcast/bcast_allcomm_nb.c						\
    src/mpi/coll/bcast/bcast_intra_binomial.c						\
    src/mpi/coll/bcast/bcast_intra_scatter_recursive_doubling_allgather.c	\
    src/mpi/coll/bcast/bcast_intra_scatter_ring_allgather.c		\
    src/mpi/coll/bcast/bcast_intra_smp.c						\
    src/mpi/coll/bcast/bcast_intra_tree.c				\
    src/mpi/coll/bcast/bcast_intra_pipelined_tree.c			\
    src/mpi/coll/bcast/bcast_inter_remote_send_local_bcast.c

noinst_HEADERS += \
    src/mpi/coll/bcast/bcast.h
