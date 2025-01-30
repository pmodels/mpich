##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/seq/hooks

libyaksa_la_SOURCES += \
	src/backend/seq/hooks/yaksuri_seq_hooks.c
