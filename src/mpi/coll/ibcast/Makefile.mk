# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/ibcast/ibcast.c

mpi_core_sources +=                        \
    src/mpi/coll/ibcast/ibcast_intra_binomial.c  \
    src/mpi/coll/ibcast/ibcast_intra_smp.c      \
    src/mpi/coll/ibcast/ibcast_inter_flat.c      \
    src/mpi/coll/ibcast/ibcast_utils.c

noinst_HEADERS += \
    src/mpi/coll/ibcast/ibcast.h
