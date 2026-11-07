##
## Copyright (C) by Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

# mpi_sources includes only the routines that are MPI function entry points
# The code for the MPI operations (e.g., MPI_SUM) is not included in
# mpi_sources

mpi_core_sources += \
    src/mpi/coll/src/coll_impl.c \
    src/mpi/coll/src/coll_fallback.c \
    src/mpi/coll/src/coll_gpu.c \
    src/mpi/coll/src/coll_greq.c \
    src/mpi/coll/src/coll_inplace.c \
    src/mpi/coll/src/coll_owned.c \
    src/mpi/coll/src/csel_load.c \
    src/mpi/coll/src/csel_search.c \
    src/mpi/coll/src/csel_validation.c \
    src/mpi/coll/src/csel_json.c \
    src/mpi/coll/src/csel_json_autogen.c
