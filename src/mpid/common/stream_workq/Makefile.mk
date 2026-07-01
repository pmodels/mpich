##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_MPID_COMMON_STREAM_WORKQ

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/stream_workq

mpi_core_sources += \
    src/mpid/common/stream_workq/stream_workq.c

endif BUILD_MPID_COMMON_STREAM_WORKQ
