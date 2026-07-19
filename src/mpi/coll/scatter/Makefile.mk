##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources

mpi_core_sources +=									\
    src/mpi/coll/scatter/scatter_allcomm_nb.c			\
    src/mpi/coll/scatter/scatter_intra_binomial.c			\
    src/mpi/coll/scatter/scatter_inter_linear.c \
    src/mpi/coll/scatter/scatter_inter_remote_send_local_scatter.c
