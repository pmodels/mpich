##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/external

noinst_HEADERS += \
	src/external/yutlist.h \
	src/external/yuthash.h
