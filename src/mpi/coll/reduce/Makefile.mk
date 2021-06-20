##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources

mpi_core_sources +=										\
    src/mpi/coll/reduce/reduce_allcomm_nb.c				\
    src/mpi/coll/reduce/reduce_intra_binomial.c				\
    src/mpi/coll/reduce/reduce_intra_reduce_scatter_gather.c			\
    src/mpi/coll/reduce/reduce_intra_smp.c			\
    src/mpi/coll/reduce/reduce_inter_local_reduce_remote_send.c
