##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

mpi_core_sources += \
   src/mpi/datatype/typerep/src/typerep_ext32.c \
   src/mpi/datatype/typerep/src/typerep_flatten.c \
   src/mpi/datatype/typerep/src/typerep_op.c

if BUILD_YAKSA_ENGINE
mpi_core_sources += \
    src/mpi/datatype/typerep/src/typerep_yaksa_create.c \
    src/mpi/datatype/typerep/src/typerep_yaksa_init.c \
    src/mpi/datatype/typerep/src/typerep_yaksa_pack.c \
    src/mpi/datatype/typerep/src/typerep_yaksa_pack_external.c \
    src/mpi/datatype/typerep/src/typerep_yaksa_iov.c \
    src/mpi/datatype/typerep/src/typerep_yaksa_commit.c \
    src/mpi/datatype/typerep/src/typerep_yaksa_debug.c
else
mpi_core_sources += \
    src/mpi/datatype/typerep/src/typerep_dataloop_create.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_init.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_pack.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_pack_external.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_iov.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_commit.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_debug.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_subarray.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_darray.c \
    src/mpi/datatype/typerep/src/typerep_dataloop_size_external.c
endif !BUILD_YAKSA_ENGINE

noinst_HEADERS += \
    src/mpi/datatype/typerep/src/typerep_internal.h   \
    src/mpi/datatype/typerep/src/typerep_pre.h
