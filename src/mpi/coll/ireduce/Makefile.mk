# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/ireduce/ireduce.c

mpi_core_sources +=                             \
    src/mpi/coll/ireduce/ireduce_intra_binomial.c		\
    src/mpi/coll/ireduce/ireduce_intra_reduce_scatter_gather.c \
    src/mpi/coll/ireduce/ireduce_intra_smp.c            \
    src/mpi/coll/ireduce/ireduce_inter_generic.c
