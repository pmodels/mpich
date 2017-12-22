# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                    \
    src/mpi/coll/ialltoallv/ialltoallv.c

mpi_core_sources +=                               \
    src/mpi/coll/ialltoallv/ialltoallv_intra_inplace.c  \
    src/mpi/coll/ialltoallv/ialltoallv_intra_blocked.c  \
    src/mpi/coll/ialltoallv/ialltoallv_inter_pairwise_exchange.c
