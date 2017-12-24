# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources
mpi_sources +=                                  \
    src/mpi/coll/iscatter/iscatter.c

mpi_core_sources +=                             \
    src/mpi/coll/iscatter/iscatter_intra_binomial.c   \
    src/mpi/coll/iscatter/iscatter_inter_linear.c \
    src/mpi/coll/iscatter/iscatter_inter_remote_send_local_scatter.c
