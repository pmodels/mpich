##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

mpi_core_sources += \
    src/mpi/datatype/typerep/src/typerep_dataloop_create.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_init.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_flatten.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_pack.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_pack_external.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_iov.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_commit.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_debug.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_subarray.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_darray.c

noinst_HEADERS += \
    src/mpi/datatype/typerep/src/typerep_internal.h
