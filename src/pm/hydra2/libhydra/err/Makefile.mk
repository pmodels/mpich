## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/err

noinst_HEADERS += libhydra/err/hydra_err.h

libhydra_la_SOURCES += \
	libhydra/err/hydra_err.c
