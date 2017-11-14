# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/ialltoall/ialltoall.c

mpi_core_sources +=                             \
    src/mpi/coll/ialltoall/ialltoall_inplace.c  \
    src/mpi/coll/ialltoall/ialltoall_brucks.c   \
    src/mpi/coll/ialltoall/ialltoall_perm_sr.c  \
    src/mpi/coll/ialltoall/ialltoall_pairwise.c
