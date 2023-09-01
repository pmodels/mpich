##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_ABI_LIB

include_HEADERS += src/binding/abi/mpi_abi.h

mpi_abi_sources += \
    src/binding/abi/mpi_abi_util.c \
    src/binding/abi/c_binding_abi.c \
    src/binding/abi/io_abi.c

endif BUILD_ABI_LIB
