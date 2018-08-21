## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/fs

noinst_HEADERS += libhydra/fs/hydra_fs.h

libhydra_la_SOURCES += \
	libhydra/fs/hydra_fs.c
