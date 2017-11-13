# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/igather/igather.c

mpi_core_sources +=                          \
    src/mpi/coll/igather/igather_binomial.c  \
    src/mpi/coll/igather/igather_short_inter.c \
    src/mpi/coll/igather/igather_long_inter.c

