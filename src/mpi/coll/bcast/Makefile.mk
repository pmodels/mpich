
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/bcast/bcast.c

mpi_core_sources +=											\
	src/mpi/coll/bcast/bcast_binomial.c						\
	src/mpi/coll/bcast/bcast_scatter_doubling_allgather.c	\
	src/mpi/coll/bcast/bcast_scatter_ring_allgather.c		\
	src/mpi/coll/bcast/bcast_mpich_tree.c

