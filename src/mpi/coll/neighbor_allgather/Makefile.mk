##
## Copyright (C) by Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources

# Currently the only algorithm for neighbor_allgather is MPIR_Coll_nb.
