##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/cuda/md

libyaksa_la_SOURCES += \
	src/backend/cuda/md/yaksuri_cudai_md.c
