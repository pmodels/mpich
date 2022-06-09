##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if BUILD_MPID_COMMON_STREAM_WORKQ

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/stream_workq

mpi_core_sources += \
    src/mpid/common/stream_workq/stream_workq.c

endif BUILD_MPID_COMMON_STREAM_WORKQ
