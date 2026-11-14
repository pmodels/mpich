##
## Copyright (C) by Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/gpu

noinst_HEADERS += src/mpid/common/gpu/mpidu_gpu_cache.h

mpi_core_sources += \
    src/mpid/common/gpu/mpidu_gpu_cache.c
