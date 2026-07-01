##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/include -I$(top_builddir)/src/frontend/include

if EMBEDDED_BUILD
noinst_HEADERS += \
	src/frontend/include/yaksa.h
else
include_HEADERS += \
	src/frontend/include/yaksa.h
endif !EMBEDDED_BUILD

noinst_HEADERS += \
	src/frontend/include/yaksi.h
