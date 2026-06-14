##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

mpi_core_sources += \
    src/mpi/group/group_impl.c \
    src/mpi/group/grouputil.c

noinst_HEADERS += src/mpi/group/group.h
