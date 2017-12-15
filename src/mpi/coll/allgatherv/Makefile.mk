
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/allgatherv/allgatherv.c

mpi_core_sources +=											\
	src/mpi/coll/allgatherv/allgatherv_recursive_doubling.c	\
	src/mpi/coll/allgatherv/allgatherv_brucks.c				\
	src/mpi/coll/allgatherv/allgatherv_ring.c
