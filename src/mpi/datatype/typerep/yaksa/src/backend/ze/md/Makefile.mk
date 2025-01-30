##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS += -I$(top_srcdir)/src/backend/ze/md

libyaksa_la_SOURCES += \
	src/backend/ze/md/yaksuri_zei_md.c
