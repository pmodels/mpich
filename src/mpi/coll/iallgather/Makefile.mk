# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                              \
    src/mpi/coll/iallgather/iallgather.c

mpi_core_sources +=                                         \
    src/mpi/coll/iallgather/iallgather_recursive_doubling.c \
    src/mpi/coll/iallgather/iallgather_brucks.c             \
    src/mpi/coll/iallgather/iallgather_ring.c
