##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources

mpi_core_sources +=										\
    src/mpi/coll/barrier/barrier_allcomm_nb.c	\
    src/mpi/coll/barrier/barrier_intra_k_dissemination.c	\
    src/mpi/coll/barrier/barrier_intra_recexch.c	\
    src/mpi/coll/barrier/barrier_intra_smp.c				\
    src/mpi/coll/barrier/barrier_inter_bcast.c
