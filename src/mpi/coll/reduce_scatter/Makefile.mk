
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/reduce_scatter/reduce_scatter.c

mpi_core_sources +=											\
	src/mpi/coll/reduce_scatter/reduce_scatter_nb.c		\
	src/mpi/coll/reduce_scatter/reduce_scatter_intra_recursive_halving.c		\
	src/mpi/coll/reduce_scatter/reduce_scatter_intra_pairwise.c				\
	src/mpi/coll/reduce_scatter/reduce_scatter_intra_recursive_doubling.c		\
	src/mpi/coll/reduce_scatter/reduce_scatter_intra_noncommutative.c				\
	src/mpi/coll/reduce_scatter/reduce_scatter_inter_generic.c
