##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/external

noinst_HEADERS += \
	src/external/yutlist.h \
	src/external/yuthash.h
