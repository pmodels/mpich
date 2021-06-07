##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources

mpi_core_sources +=											\
    src/mpi/coll/allreduce/allreduce.c \
    src/mpi/coll/allreduce/allreduce_allcomm_nb.c	\
    src/mpi/coll/allreduce/allreduce_intra_recursive_doubling.c	\
    src/mpi/coll/allreduce/allreduce_intra_reduce_scatter_allgather.c	\
    src/mpi/coll/allreduce/allreduce_intra_smp.c	\
    src/mpi/coll/allreduce/allreduce_inter_reduce_exchange_bcast.c	\
    src/mpi/coll/allreduce/allreduce_init.c
