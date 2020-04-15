##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in 
# mpi_sources
mpi_sources += \
    src/mpi/coll/ineighbor_alltoall/ineighbor_alltoall.c

mpi_core_sources += \
    src/mpi/coll/ineighbor_alltoall/ineighbor_alltoall_allcomm_sched_linear.c			\
    src/mpi/coll/ineighbor_alltoall/ineighbor_alltoall_gentran_algos.c			\
    src/mpi/coll/ineighbor_alltoall/ineighbor_alltoall_allcomm_gentran_linear.c
