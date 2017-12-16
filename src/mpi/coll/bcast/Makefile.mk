
# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                     		\
	src/mpi/coll/bcast/bcast.c

mpi_core_sources +=											\
	src/mpi/coll/bcast/bcast_utils.c						\
	src/mpi/coll/bcast/bcast_nb.c						\
	src/mpi/coll/bcast/bcast_intra_binomial.c						\
	src/mpi/coll/bcast/bcast_intra_scatter_doubling_allgather.c	\
	src/mpi/coll/bcast/bcast_intra_scatter_ring_allgather.c		\
	src/mpi/coll/bcast/bcast_intra_smp.c						\
	src/mpi/coll/bcast/bcast_inter_generic.c

noinst_HEADERS += \
    src/mpi/coll/bcast/bcast.h
