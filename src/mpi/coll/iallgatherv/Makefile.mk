# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                              \
    src/mpi/coll/iallgatherv/iallgatherv.c

mpi_core_sources +=                                           \
    src/mpi/coll/iallgatherv/iallgatherv_recursive_doubling.c \
    src/mpi/coll/iallgatherv/iallgatherv_brucks.c             \
    src/mpi/coll/iallgatherv/iallgatherv_ring.c               \
    src/mpi/coll/iallgatherv/iallgatherv_generic_inter.c
