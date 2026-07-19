##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/ch4/netmod/include

noinst_HEADERS += src/mpid/ch4/netmod/include/netmod.h
noinst_HEADERS += src/mpid/ch4/netmod/include/netmod_impl.h

include $(top_srcdir)/src/mpid/ch4/netmod/src/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/netmod/ofi/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/netmod/ucx/Makefile.mk
include $(top_srcdir)/src/mpid/ch4/netmod/stubnm/Makefile.mk
