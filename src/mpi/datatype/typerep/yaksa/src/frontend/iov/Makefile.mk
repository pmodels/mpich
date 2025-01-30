##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/iov

libyaksa_la_SOURCES += \
	src/frontend/iov/yaksa_iov_len.c \
	src/frontend/iov/yaksa_iov_len_max.c \
	src/frontend/iov/yaksa_iov.c
