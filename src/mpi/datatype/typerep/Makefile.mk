##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/src/mpi/datatype/typerep/dataloop/Makefile.mk

mpi_core_sources += \
    src/mpi/datatype/typerep/src/typerep_flatten.c \
    src/mpi/datatype/typerep/src/typerep_pack.c \
    src/mpi/datatype/typerep/src/typerep_pack_external.c \
    src/mpi/datatype/typerep/src/typerep_iov.c \
    src/mpi/datatype/typerep/src/typerep_create.c \
    src/mpi/datatype/typerep/src/typerep_debug.c
