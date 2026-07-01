##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/iov

libyaksa_la_SOURCES += \
	src/frontend/iov/yaksa_iov_len.c \
	src/frontend/iov/yaksa_iov_len_max.c \
	src/frontend/iov/yaksa_iov.c
