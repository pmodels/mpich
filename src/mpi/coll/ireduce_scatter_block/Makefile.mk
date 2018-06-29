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
mpi_sources +=                                  \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block.c

mpi_core_sources +=                          \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_recursive_halving.c  \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_pairwise.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_recursive_doubling.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_noncommutative.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_inter_remote_reduce_local_scatterv.c \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_gentran_algos.c                    \
    src/mpi/coll/ireduce_scatter_block/ireduce_scatter_block_intra_recexch.c
