# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/ialltoall/ialltoall.c

mpi_core_sources +=                             \
    src/mpi/coll/ialltoall/ialltoall_intra_inplace.c  \
    src/mpi/coll/ialltoall/ialltoall_intra_brucks.c   \
    src/mpi/coll/ialltoall/ialltoall_intra_permuted_sendrecv.c  \
    src/mpi/coll/ialltoall/ialltoall_intra_pairwise.c \
    src/mpi/coll/ialltoall/ialltoall_inter_generic.c
