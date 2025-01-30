##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/hip/md

libyaksa_la_SOURCES += \
	src/backend/hip/md/yaksuri_hipi_md.c
