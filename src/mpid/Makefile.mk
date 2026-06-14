##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/include

noinst_HEADERS +=                          \
    src/mpid/include/mpidu_pre.h


include $(top_srcdir)/src/mpid/ch3/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/Makefile.mk
include $(top_srcdir)/src/mpid/common/Makefile.mk
