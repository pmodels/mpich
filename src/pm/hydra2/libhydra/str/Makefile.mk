## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/str

noinst_HEADERS += libhydra/str/hydra_str.h

libhydra_la_SOURCES += \
	libhydra/str/hydra_str.c
