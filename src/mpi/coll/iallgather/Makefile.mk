# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                              \
    src/mpi/coll/iallgather/iallgather.c

mpi_core_sources +=                                         \
    src/mpi/coll/iallgather/iallgather_intra_recursive_doubling.c \
    src/mpi/coll/iallgather/iallgather_intra_brucks.c             \
    src/mpi/coll/iallgather/iallgather_intra_ring.c               \
    src/mpi/coll/iallgather/iallgather_inter_local_gather_remote_bcast.c

