## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/arg

noinst_HEADERS += libhydra/arg/hydra_arg.h

libhydra_la_SOURCES += \
	libhydra/arg/hydra_arg.c
