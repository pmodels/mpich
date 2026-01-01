##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_ABI_LIB

mpi_abi_sources += \
    src/binding/abi/c_binding_abi.c \
    src/binding/abi/io_abi.c

mpi_abi_core_sources += \
    src/binding/abi/mpi_abi_util.c

endif BUILD_ABI_LIB
