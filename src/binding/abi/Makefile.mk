##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_ABI_LIB

include_HEADERS += src/binding/abi/mpi_abi.h

mpi_abi_sources += \
    src/binding/abi/c_binding_abi.c \
    src/binding/abi/io_abi.c

mpi_abi_core_sources += \
    src/binding/abi/mpi_abi_util.c

endif BUILD_ABI_LIB
