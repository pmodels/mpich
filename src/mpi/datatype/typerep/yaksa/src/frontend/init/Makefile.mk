##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/init

libyaksa_la_SOURCES += \
	src/frontend/init/yaksa_init.c
