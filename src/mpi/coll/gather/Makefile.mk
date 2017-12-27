
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/gather/gather.c

mpi_core_sources +=									\
	src/mpi/coll/gather/gather_intra_binomial_indexed.c			\
	src/mpi/coll/gather/gather_intra_heterogeneous.c			\
	src/mpi/coll/gather/gather_nb.c			\
	src/mpi/coll/gather/gather_intra_binomial.c			\
	src/mpi/coll/gather/gather_inter_generic.c
