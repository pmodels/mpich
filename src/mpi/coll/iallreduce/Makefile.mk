# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                              \
    src/mpi/coll/iallreduce/iallreduce.c

mpi_core_sources +=                                         \
    src/mpi/coll/iallreduce/iallreduce_naive.c              \
    src/mpi/coll/iallreduce/iallreduce_redscat_allgather.c  \
    src/mpi/coll/iallreduce/iallreduce_recursive_doubling.c \
    src/mpi/coll/iallreduce/iallreduce_generic_inter.c
