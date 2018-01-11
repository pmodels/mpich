# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block.c

mpi_core_sources +=                          \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_recursive_halving.c  \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_pairwise.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_recursive_doubling.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_noncommutative.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_inter_remote_reduce_local_scatterv.c
