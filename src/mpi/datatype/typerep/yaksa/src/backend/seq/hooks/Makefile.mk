##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/seq/hooks

libyaksa_la_SOURCES += \
	src/backend/seq/hooks/yaksuri_seq_hooks.c
