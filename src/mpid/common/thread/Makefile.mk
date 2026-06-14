##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if BUILD_MPID_COMMON_THREAD

# so that clients can successfully include mpid_thread.h
AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/common/thread

noinst_HEADERS += src/mpid/common/thread/mpidu_thread_fallback.h

endif BUILD_MPID_COMMON_THREAD
