
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/allgather/allgather.c

mpi_core_sources +=											\
	src/mpi/coll/allgather/allgather_intra_recursive_doubling.c	\
	src/mpi/coll/allgather/allgather_intra_brucks.c				\
	src/mpi/coll/allgather/allgather_intra_ring.c					\
	src/mpi/coll/allgather/allgather_inter_generic.c
