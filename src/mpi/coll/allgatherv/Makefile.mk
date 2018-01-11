
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/allgatherv/allgatherv.c

mpi_core_sources +=											\
	src/mpi/coll/allgatherv/allgatherv_nb.c	\
	src/mpi/coll/allgatherv/allgatherv_intra_recursive_doubling.c	\
	src/mpi/coll/allgatherv/allgatherv_intra_brucks.c				\
	src/mpi/coll/allgatherv/allgatherv_intra_ring.c				\
	src/mpi/coll/allgatherv/allgatherv_inter_remote_gather_local_bcast.c
