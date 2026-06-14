##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/md

libyaksa_la_SOURCES += \
	src/backend/hip/md/yaksuri_hipi_md.c
