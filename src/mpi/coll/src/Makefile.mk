##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources += \
    src/mpi/coll/src/coll_impl.c \
    src/mpi/coll/src/csel.c \
    src/mpi/coll/src/csel_container.c \
    src/mpi/coll/src/csel_json_autogen.c
