##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/frontend/bounds

libyaksa_la_SOURCES += \
	src/frontend/bounds/yaksa_bounds.c
