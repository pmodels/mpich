## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/signal

noinst_HEADERS += libhydra/signal/hydra_signal.h

libhydra_la_SOURCES += \
	libhydra/signal/hydra_signal.c
