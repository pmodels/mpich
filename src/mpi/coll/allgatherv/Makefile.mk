## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##
##  Portions of this code were written by Intel Corporation.
##  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
##  to Argonne National Laboratory subject to Software Grant and Corporate
##  Contributor License Agreement dated February 8, 2012.

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources +=                             \
    src/mpi/coll/allgatherv/allgatherv.c

mpi_core_sources +=											\
    src/mpi/coll/allgatherv/allgatherv_allcomm_nb.c	\
    src/mpi/coll/allgatherv/allgatherv_intra_recursive_doubling.c	\
    src/mpi/coll/allgatherv/allgatherv_intra_brucks.c				\
    src/mpi/coll/allgatherv/allgatherv_intra_ring.c				\
    src/mpi/coll/allgatherv/allgatherv_inter_remote_gather_local_bcast.c
