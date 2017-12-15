# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/ired_scat_block/ired_scat_block.c

mpi_core_sources +=                          \
    src/mpi/coll/ired_scat_block/ired_scat_block_rec_hlv.c  \
    src/mpi/coll/ired_scat_block/ired_scat_block_pairwise.c \
    src/mpi/coll/ired_scat_block/ired_scat_block_recursive_doubling.c \
    src/mpi/coll/ired_scat_block/ired_scat_block_noncomm.c \
    src/mpi/coll/ired_scat_block/ired_scat_block_default_inter.c

