##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

mpi_core_sources += \
    src/mpi/comm/comm_impl.c \
    src/mpi/comm/comm_split.c \
    src/mpi/comm/comm_split_type.c \
    src/mpi/comm/comm_split_type_nbhd.c \
    src/mpi/comm/ulfm_impl.c \
    src/mpi/comm/builtin_comms.c \
    src/mpi/comm/commutil.c \
    src/mpi/comm/contextid.c

noinst_HEADERS += src/mpi/comm/mpicomm.h
