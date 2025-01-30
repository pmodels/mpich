##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/bounds

libyaksa_la_SOURCES += \
	src/frontend/bounds/yaksa_bounds.c
