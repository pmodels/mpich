##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/md

libyaksa_la_SOURCES += \
	src/backend/cuda/md/yaksuri_cudai_md.c
