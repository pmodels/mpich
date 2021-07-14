##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources +=                             \
    src/mpi/coll/iscatter/iscatter_intra_sched_binomial.c   \
    src/mpi/coll/iscatter/iscatter_inter_sched_linear.c \
    src/mpi/coll/iscatter/iscatter_inter_sched_remote_send_local_scatter.c \
    src/mpi/coll/iscatter/iscatter_tsp_tree.c
