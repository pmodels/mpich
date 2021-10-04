##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=											\
    src/mpi/coll/reduce_scatter/reduce_scatter_allcomm_nb.c		\
    src/mpi/coll/reduce_scatter/reduce_scatter_intra_recursive_halving.c		\
    src/mpi/coll/reduce_scatter/reduce_scatter_intra_pairwise.c				\
    src/mpi/coll/reduce_scatter/reduce_scatter_intra_recursive_doubling.c		\
    src/mpi/coll/reduce_scatter/reduce_scatter_intra_noncommutative.c				\
    src/mpi/coll/reduce_scatter/reduce_scatter_inter_remote_reduce_local_scatter.c
