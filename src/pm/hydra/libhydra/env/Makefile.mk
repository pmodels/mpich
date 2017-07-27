## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/env

noinst_HEADERS += libhydra/env/hydra_env.h

libhydra_la_SOURCES += \
	libhydra/env/hydra_env.c
