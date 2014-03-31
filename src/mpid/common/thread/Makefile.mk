### -*- Mode: Makefile; -*-
### vim: set ft=automake :
###
### (C) 2011 by Argonne National Laboratory.
###     See COPYRIGHT in top-level directory.
###

if BUILD_MPID_COMMON_THREAD
if THREAD_SERIALIZED_OR_MULTIPLE

# so that clients can successfully include mpid_thread.h
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/thread

noinst_HEADERS += src/mpid/common/thread/mpid_thread.h
mpi_core_sources += src/mpid/common/thread/mpid_thread.c

endif THREAD_SERIALIZED_OR_MULTIPLE
endif BUILD_MPID_COMMON_THREAD

