##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/info

libyaksa_la_SOURCES += \
	src/frontend/info/yaksa_info.c
