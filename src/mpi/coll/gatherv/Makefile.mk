
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/gatherv/gatherv.c

mpi_core_sources +=									\
	src/mpi/coll/gatherv/gatherv_nb.c \
	src/mpi/coll/gatherv/gatherv_inter_generic.c \
	src/mpi/coll/gatherv/gatherv_intra_linear.c \
	src/mpi/coll/gatherv/gatherv_intra_linear_ssend.c
