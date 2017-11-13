
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/red_scat/red_scat.c

mpi_core_sources +=											\
	src/mpi/coll/red_scat/red_scat_recursive_halving.c		\
	src/mpi/coll/red_scat/red_scat_pairwise.c				\
	src/mpi/coll/red_scat/red_scat_recursive_doubling.c		\
	src/mpi/coll/red_scat/red_scat_noncomm.c				\
	src/mpi/coll/red_scat/red_scat_generic_inter.c

