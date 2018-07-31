## -*- Mode: Makefile; -*-
##
## (C) 2017 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

AM_CPPFLAGS += -I$(top_srcdir)/libhydra/spawn

noinst_HEADERS += libhydra/spawn/hydra_spawn.h

libhydra_la_SOURCES += \
	libhydra/spawn/hydra_spawn.c
