##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
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
