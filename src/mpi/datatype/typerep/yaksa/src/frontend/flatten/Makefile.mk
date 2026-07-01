##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/flatten

libyaksa_la_SOURCES += \
	src/frontend/flatten/yaksa_flatten_size.c \
	src/frontend/flatten/yaksa_flatten.c \
	src/frontend/flatten/yaksa_unflatten.c
