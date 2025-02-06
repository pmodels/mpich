##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/flatten

libyaksa_la_SOURCES += \
	src/frontend/flatten/yaksa_flatten_size.c \
	src/frontend/flatten/yaksa_flatten.c \
	src/frontend/flatten/yaksa_unflatten.c
