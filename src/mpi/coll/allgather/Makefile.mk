
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/allgather/allgather.c

mpi_core_sources +=											\
	src/mpi/coll/allgather/allgather_recursive_doubling.c	\
	src/mpi/coll/allgather/allgather_brucks.c				\
	src/mpi/coll/allgather/allgather_ring.c					\
	src/mpi/coll/allgather/allgather_generic_inter.c
