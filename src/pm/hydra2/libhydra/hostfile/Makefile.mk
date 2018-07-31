## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/hostfile

noinst_HEADERS += libhydra/hostfile/hydra_hostfile.h

libhydra_la_SOURCES += \
	libhydra/hostfile/hydra_hostfile.c
